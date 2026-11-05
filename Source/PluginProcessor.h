// YMY DELAY - BY ORYMY / YMY RECORDS
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "DSP/DelayEngine.h"
#include "DSP/TempoSync.h"
#include "Parameters/Parameters.h"
#include "Presets/PresetManager.h"

class YMYDelayAudioProcessor : public juce::AudioProcessor
{
public:
    YMYDelayAudioProcessor();
    ~YMYDelayAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "YMY DELAY"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Acceso para la interfaz (hilo de mensajes).
    juce::AudioProcessorValueTreeState apvts;
    ymy::PresetManager presets;

    double getUiBpm() const noexcept      { return uiBpm.load(); }
    int    getUiSource() const noexcept   { return uiSource.load(); }
    double getUiTimeMs() const noexcept   { return timeCtrl.getTimeMs(); }
    bool   getUiFolded() const noexcept   { return uiFolded.load(); }
    float  takeMeterL() noexcept          { return meterL.exchange (0.0f); }
    float  takeMeterR() noexcept          { return meterR.exchange (0.0f); }

    void handleTap();
    void setParameterPlain (const char* id, float plainValue);

    int  getGuiScaleIndex() const;
    void setGuiScaleIndex (int index);

private:
    ymy::dsp::DelayEngine engine;
    ymy::dsp::DelayTimeController timeCtrl;
    ymy::dsp::HostTempoReader tempoReader;
    ymy::dsp::TapTempo tapTempo;

    std::atomic<float> *pOn {}, *pSync {}, *pNote {}, *pFreeMs {}, *pManualBpm {}, *pFeedback {},
                       *pMix {}, *pTone {}, *pWidth {}, *pDuck {}, *pMode {}, *pAnalog {},
                       *pFilter {}, *pHp {}, *pLp {}, *pOutput {};

    std::atomic<double> uiBpm { 120.0 };
    std::atomic<int>    uiSource { (int) ymy::dsp::TempoInfo::Source::Manual };
    std::atomic<bool>   uiFolded { false };
    std::atomic<float>  meterL { 0.0f }, meterR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YMYDelayAudioProcessor)
};
