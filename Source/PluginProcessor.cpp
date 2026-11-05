#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
void raiseMeter (std::atomic<float>& m, float v) noexcept
{
    if (v > m.load())
        m.store (v);
}
} // namespace

YMYDelayAudioProcessor::YMYDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "YMY_DELAY_STATE", ymy::params::createLayout()),
      presets (apvts)
{
    namespace P = ymy::params;
    pOn        = apvts.getRawParameterValue (P::kOn);
    pSync      = apvts.getRawParameterValue (P::kSync);
    pNote      = apvts.getRawParameterValue (P::kNote);
    pFreeMs    = apvts.getRawParameterValue (P::kFreeMs);
    pManualBpm = apvts.getRawParameterValue (P::kManualBpm);
    pFeedback  = apvts.getRawParameterValue (P::kFeedback);
    pMix       = apvts.getRawParameterValue (P::kMix);
    pTone      = apvts.getRawParameterValue (P::kTone);
    pWidth     = apvts.getRawParameterValue (P::kWidth);
    pDuck      = apvts.getRawParameterValue (P::kDuck);
    pMode      = apvts.getRawParameterValue (P::kMode);
    pAnalog    = apvts.getRawParameterValue (P::kAnalog);
    pFilter    = apvts.getRawParameterValue (P::kFilter);
    pHp        = apvts.getRawParameterValue (P::kHp);
    pLp        = apvts.getRawParameterValue (P::kLp);
    pOutput    = apvts.getRawParameterValue (P::kOutput);
}

//==============================================================================
void YMYDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    timeCtrl.prepare (sampleRate, ymy::dsp::DelayEngine::kMinDelayMs, ymy::dsp::DelayEngine::kMaxDelayMs, 50.0);
    tempoReader.reset();
}

void YMYDelayAudioProcessor::releaseResources() {}

bool YMYDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

double YMYDelayAudioProcessor::getTailLengthSeconds() const
{
    const double t  = timeCtrl.getTimeMs() * 0.001;
    const double fb = juce::jlimit (0.01, 0.9, (double) pFeedback->load() * 0.01);
    return juce::jlimit (0.5, 20.0, t * (std::ceil (std::log (0.001) / std::log (fb)) + 1.0));
}

//==============================================================================
void YMYDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    ymy::dsp::EngineParams ep;
    ep.on       = pOn->load() > 0.5f;
    ep.feedback = pFeedback->load() * 0.01f;
    ep.mix      = pMix->load() * 0.01f;
    ep.tone     = pTone->load() * 0.01f;
    ep.width    = pWidth->load() * 0.01f;
    ep.duck     = pDuck->load() * 0.01f;
    ep.mode     = (ymy::dsp::DelayMode) juce::jlimit (0, 2, (int) (pMode->load() + 0.5f));
    ep.analog   = pAnalog->load() > 0.5f;
    ep.filterOn = pFilter->load() > 0.5f;
    ep.hpHz     = pHp->load();
    ep.lpHz     = pLp->load();
    ep.outputDb = pOutput->load();

    // Host Tempo Sync: BPM real del transporte del host (getPlayHead()).
    const auto tempo = tempoReader.read (getPlayHead(), (double) pManualBpm->load());
    uiBpm.store (tempo.bpm);
    uiSource.store ((int) tempo.source);

    const auto division = (ymy::dsp::NoteDivision) juce::jlimit (0, ymy::dsp::kNumDivisions - 1,
                                                                 (int) (pNote->load() + 0.5f));

    timeCtrl.updateTarget (pSync->load() > 0.5f, division, tempo.bpm, (double) pFreeMs->load());
    uiFolded.store (timeCtrl.wasFolded());

    engine.process (buffer, ep, timeCtrl);

    raiseMeter (meterL, engine.getLastPeakL());
    raiseMeter (meterR, engine.getLastPeakR());
}

//==============================================================================
juce::AudioProcessorEditor* YMYDelayAudioProcessor::createEditor()
{
    return new YMYDelayAudioProcessorEditor (*this);
}

void YMYDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void YMYDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
void YMYDelayAudioProcessor::setParameterPlain (const char* id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

// TAP: sin BPM del host -> fija el BPM manual. Con host -> fija el tiempo FREE y desactiva SYNC.
void YMYDelayAudioProcessor::handleTap()
{
    const double bpm = tapTempo.tap();
    if (bpm <= 0.0)
        return;

    if (uiSource.load() == (int) ymy::dsp::TempoInfo::Source::Manual)
    {
        setParameterPlain (ymy::params::kManualBpm, (float) bpm);
    }
    else
    {
        double ms = 60000.0 / bpm;
        while (ms > ymy::dsp::DelayEngine::kMaxDelayMs)
            ms *= 0.5;

        setParameterPlain (ymy::params::kFreeMs, (float) ms);
        setParameterPlain (ymy::params::kSync, 0.0f);
    }
}

int YMYDelayAudioProcessor::getGuiScaleIndex() const
{
    return juce::jlimit (0, 7, (int) apvts.state.getProperty ("guiScale", ymy::params::kDefaultGuiScaleIndex));
}

void YMYDelayAudioProcessor::setGuiScaleIndex (int index)
{
    apvts.state.setProperty ("guiScale", juce::jlimit (0, 7, index), nullptr);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YMYDelayAudioProcessor();
}
