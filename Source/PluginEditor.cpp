#include "PluginEditor.h"

#include "Utilities/Version.h"

using namespace ymy::ui;

//==============================================================================
YMYPanel::YMYPanel (YMYDelayAudioProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    setSize (kBaseW, kBaseH);

    namespace P = ymy::params;

    initKnob (timeK,     "TIME",     P::kFreeMs);
    initKnob (feedbackK, "FEEDBACK", P::kFeedback);
    initKnob (mixK,      "MIX",      P::kMix);
    initKnob (toneK,     "TONE",     P::kTone);
    initKnob (widthK,    "WIDTH",    P::kWidth);
    initKnob (duckK,     "DUCK",     P::kDuck);

    initHSlider (hpH,  "HP",  P::kHp,        60);
    initHSlider (lpH,  "LP",  P::kLp,        66);
    initHSlider (bpmH, "BPM", P::kManualBpm, 74);
    initHSlider (outH, "OUT", P::kOutput,    66);

    initToggle (onBtn,     onAtt,     P::kOn);
    initToggle (syncBtn,   syncAtt,   P::kSync);
    initToggle (analogBtn, analogAtt, P::kAnalog);
    initToggle (filterBtn, filterAtt, P::kFilter);

    noteBox.addItemList (ymy::dsp::divisionLabels(), 1);
    noteAtt = std::make_unique<ComboAtt> (proc.apvts, P::kNote, noteBox);
    addAndMakeVisible (noteBox);

    modeBox.addItemList (P::modeLabels(), 1);
    modeAtt = std::make_unique<ComboAtt> (proc.apvts, P::kMode, modeBox);
    addAndMakeVisible (modeBox);

    scaleBox.addItemList (P::guiScaleLabels(), 1);
    scaleBox.setSelectedId (proc.getGuiScaleIndex() + 1, juce::dontSendNotification);
    scaleBox.onChange = [this]
    {
        const int idx = scaleBox.getSelectedId() - 1;
        if (idx >= 0)
        {
            proc.setGuiScaleIndex (idx);
            if (onScaleChanged)
                onScaleChanged (idx);
        }
    };
    addAndMakeVisible (scaleBox);

    initDisplay (bpmLabel);
    initDisplay (timeLabel);

    tapBtn.onClick = [this] { proc.handleTap(); };
    addAndMakeVisible (tapBtn);

    // Presets
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this] { onPresetChosen(); };
    rebuildPresetMenu();

    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    saveBtn.onClick = [this]
    {
        const auto initial = proc.presets.isCurrentUser() ? proc.presets.getCurrentName() : juce::String ("My Preset");
        promptForName ("Save preset", initial, [this] (juce::String name)
        {
            if (proc.presets.saveUser (name))
                rebuildPresetMenu();
            else
                showInfo ("Save preset", "The preset could not be saved.");
        });
    };
    addAndMakeVisible (saveBtn);

    renameBtn.onClick = [this]
    {
        if (! proc.presets.isCurrentUser())
        {
            showInfo ("Rename preset", "Only user presets can be renamed. Load or save a user preset first.");
            return;
        }

        const auto oldName = proc.presets.getCurrentName();
        promptForName ("Rename preset", oldName, [this, oldName] (juce::String name)
        {
            if (proc.presets.renameUser (oldName, name))
                rebuildPresetMenu();
            else
                showInfo ("Rename preset", "The preset could not be renamed (empty or already existing name).");
        });
    };
    addAndMakeVisible (renameBtn);

    deleteBtn.onClick = [this]
    {
        if (! proc.presets.isCurrentUser())
        {
            showInfo ("Delete preset", "Only user presets can be deleted.");
            return;
        }

        const auto name = proc.presets.getCurrentName();
        juce::Component::SafePointer<YMYPanel> safe (this);

        juce::AlertWindow::showAsync (
            juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon,
                                                          "Delete preset",
                                                          "Delete user preset \"" + name + "\"?",
                                                          "Delete", "Cancel", this),
            [safe, name] (int result)
            {
                if (result == 1 && safe != nullptr)
                {
                    safe->proc.presets.deleteUser (name);
                    safe->rebuildPresetMenu();
                }
            });
    };
    addAndMakeVisible (deleteBtn);

    addAndMakeVisible (onBtn);
    addAndMakeVisible (syncBtn);
    addAndMakeVisible (analogBtn);
    addAndMakeVisible (filterBtn);

    startTimerHz (30);
    timerCallback();
}

YMYPanel::~YMYPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void YMYPanel::initKnob (Knob& k, const juce::String& text, const char* id)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 20);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f, true);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);

    if (auto* prm = proc.apvts.getParameter (id))
        k.slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (font (13.0f, juce::Font::bold));
    k.label.setColour (juce::Label::textColourId, kGold);

    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
}

void YMYPanel::initHSlider (HSlider& h, const juce::String& text, const char* id, int textBoxWidth)
{
    h.slider.setSliderStyle (juce::Slider::LinearHorizontal);
    h.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, textBoxWidth, 20);
    h.att = std::make_unique<SliderAtt> (proc.apvts, id, h.slider);

    if (auto* prm = proc.apvts.getParameter (id))
        h.slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));

    h.label.setText (text, juce::dontSendNotification);
    h.label.setJustificationType (juce::Justification::centredLeft);
    h.label.setFont (font (12.0f, juce::Font::bold));
    h.label.setColour (juce::Label::textColourId, kGold);

    addAndMakeVisible (h.slider);
    addAndMakeVisible (h.label);
}

void YMYPanel::initToggle (juce::ToggleButton& b, std::unique_ptr<ButtonAtt>& att, const char* id)
{
    att = std::make_unique<ButtonAtt> (proc.apvts, id, b);
}

void YMYPanel::initDisplay (juce::Label& l)
{
    l.setJustificationType (juce::Justification::centredLeft);
    l.setFont (font (13.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, kText);
    l.setColour (juce::Label::backgroundColourId, kPanel);
    l.setColour (juce::Label::outlineColourId, kGoldDim);
    l.setBorderSize (juce::BorderSize<int> (0, 8, 0, 8));
    addAndMakeVisible (l);
}

//==============================================================================
void YMYPanel::resized()
{
    // Cabecera
    scaleBox.setBounds (560, 14, 84, 26);
    onBtn.setBounds (660, 14, 96, 26);

    // Presets
    prevBtn.setBounds (24, 70, 28, 26);
    presetBox.setBounds (56, 70, 236, 26);
    nextBtn.setBounds (296, 70, 28, 26);
    saveBtn.setBounds (334, 70, 60, 26);
    renameBtn.setBounds (398, 70, 70, 26);
    deleteBtn.setBounds (472, 70, 66, 26);

    // Perillas principales
    Knob* knobs[] = { &timeK, &feedbackK, &mixK, &toneK, &widthK, &duckK };
    for (int i = 0; i < 6; ++i)
    {
        const int x = 30 + i * 122;
        knobs[i]->label.setBounds (x, 112, 114, 18);
        knobs[i]->slider.setBounds (x, 130, 114, 136);
    }

    // Sincronizacion
    syncBtn.setBounds (30, 292, 74, 28);
    noteBox.setBounds (112, 292, 120, 28);
    tapBtn.setBounds (240, 292, 60, 28);
    bpmLabel.setBounds (312, 292, 220, 28);
    timeLabel.setBounds (540, 292, 110, 28);
    modeBox.setBounds (660, 292, 96, 28);

    analogBtn.setBounds (30, 326, 84, 26);
    filterBtn.setBounds (122, 326, 84, 26);
    hpH.label.setBounds (218, 326, 26, 26);
    hpH.slider.setBounds (244, 326, 190, 26);
    lpH.label.setBounds (448, 326, 26, 26);
    lpH.slider.setBounds (474, 326, 190, 26);

    bpmH.label.setBounds (30, 358, 36, 26);
    bpmH.slider.setBounds (66, 358, 230, 26);
    outH.label.setBounds (316, 358, 34, 26);
    outH.slider.setBounds (350, 358, 200, 26);
}

void YMYPanel::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1b1e), 0.0f, 0.0f,
                                             juce::Colour (0xff0f0f11), 0.0f, (float) kBaseH, false));
    g.fillAll();

    g.setColour (kGold);
    g.fillRect (0, 0, kBaseW, 3);

    // Identidad
    g.setColour (kGold);
    g.setFont (font (30.0f, juce::Font::bold));
    g.drawText ("YMY DELAY", 24, 8, 280, 34, juce::Justification::centredLeft, false);

    g.setColour (kText.withAlpha (0.75f));
    g.setFont (font (11.0f, juce::Font::bold));
    g.drawText ("VINTAGE BACHATA DELAY", 26, 42, 260, 16, juce::Justification::centredLeft, false);

    g.setColour (kGoldDim);
    g.setFont (font (11.0f, juce::Font::bold));
    g.drawText ("BY ORYMY  -  YMY RECORDS", 320, 20, 230, 16, juce::Justification::centredLeft, false);

    g.setFont (font (10.0f, juce::Font::bold));
    g.drawText ("THE CLASSIC BACHATA ECHO.", 540, 74, 216, 18, juce::Justification::centredRight, false);

    // Paneles
    g.setColour (kPanel);
    g.fillRoundedRectangle (20.0f, 106.0f, 740.0f, 170.0f, 10.0f);
    g.fillRoundedRectangle (20.0f, 282.0f, 740.0f, 112.0f, 10.0f);
    g.setColour (kGoldDim.withAlpha (0.55f));
    g.drawRoundedRectangle (20.0f, 106.0f, 740.0f, 170.0f, 10.0f, 1.0f);
    g.drawRoundedRectangle (20.0f, 282.0f, 740.0f, 112.0f, 10.0f, 1.0f);

    // Medidores de salida
    g.setColour (kGold);
    g.setFont (font (12.0f, juce::Font::bold));
    g.drawText ("LEVEL", 560, 354, 38, 34, juce::Justification::centredLeft, false);

    auto drawMeter = [&g] (float level, float y)
    {
        const juce::Rectangle<float> r (604.0f, y, 146.0f, 8.0f);
        g.setColour (juce::Colour (0xff222225));
        g.fillRoundedRectangle (r, 3.0f);

        const float dB  = juce::Decibels::gainToDecibels (level, -60.0f);
        const float pos = juce::jlimit (0.0f, 1.0f, (dB + 60.0f) / 60.0f);
        g.setColour (dB > -0.5f ? juce::Colours::red : kGold);
        g.fillRoundedRectangle (r.withWidth (r.getWidth() * pos), 3.0f);
    };

    drawMeter (meterL, 361.0f);
    drawMeter (meterR, 374.0f);

    // Pie
    g.setColour (kGoldDim);
    g.setFont (font (10.5f, juce::Font::bold));
    g.drawText ("MADE FOR BACHATA.  DESIGNED BY ORYMY.  YMY RECORDS.", 0, 408, kBaseW, 18, juce::Justification::centred, false);
    g.setFont (font (10.0f));
    g.drawText (juce::String ("v") + ymy::kVersion, kBaseW - 90, 408, 70, 18, juce::Justification::centredRight, false);
}

//==============================================================================
void YMYPanel::timerCallback()
{
    const int  src      = proc.getUiSource();
    const bool fromHost = (src != (int) ymy::dsp::TempoInfo::Source::Manual);
    const bool sync     = syncBtn.getToggleState();

    juce::String bpmText;
    if (src == (int) ymy::dsp::TempoInfo::Source::Host)
        bpmText = "HOST BPM  " + juce::String (proc.getUiBpm(), 1);
    else if (src == (int) ymy::dsp::TempoInfo::Source::HeldFromHost)
        bpmText = "HOST BPM  " + juce::String (proc.getUiBpm(), 1) + "  (LAST)";
    else
        bpmText = "BPM  " + juce::String (proc.getUiBpm(), 1) + "  (MANUAL)";

    juce::String timeText = "TIME  " + juce::String (proc.getUiTimeMs(), 1) + " ms";
    if (sync && proc.getUiFolded())
        timeText << "*";

    if (bpmText != lastBpmText)   { lastBpmText = bpmText;   bpmLabel.setText (bpmText, juce::dontSendNotification); }
    if (timeText != lastTimeText) { lastTimeText = timeText; timeLabel.setText (timeText, juce::dontSendNotification); }

    timeK.slider.setEnabled (! sync);
    noteBox.setEnabled (sync);
    bpmH.slider.setEnabled (! fromHost);

    meterL = std::max (proc.takeMeterL(), meterL * 0.82f);
    meterR = std::max (proc.takeMeterR(), meterR * 0.82f);
    repaint (600, 356, 156, 30);
}

//==============================================================================
void YMYPanel::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);

    const auto& factory = ymy::PresetManager::factoryPresets();
    juce::String lastCategory;

    for (size_t i = 0; i < factory.size(); ++i)
    {
        if (lastCategory != factory[i].category)
        {
            lastCategory = factory[i].category;
            presetBox.addSectionHeading (lastCategory);
        }
        presetBox.addItem (factory[i].name, (int) i + 1);
    }

    const auto users = proc.presets.getUserPresetNames();
    if (! users.isEmpty())
    {
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < users.size(); ++i)
            presetBox.addItem (users[i], 1001 + i);
    }

    const auto current = proc.presets.getCurrentName();
    int id = 0;

    if (proc.presets.isCurrentUser())
    {
        const int idx = users.indexOf (current);
        if (idx >= 0)
            id = 1001 + idx;
    }
    else
    {
        for (size_t i = 0; i < factory.size(); ++i)
            if (current == factory[i].name)
                id = (int) i + 1;
    }

    if (id > 0)
        presetBox.setSelectedId (id, juce::dontSendNotification);
    else
        presetBox.setText (current.isEmpty() ? juce::String ("- INIT -") : current, juce::dontSendNotification);
}

void YMYPanel::onPresetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0)
        return;

    if (id < 1000)
    {
        proc.presets.loadFactory (id - 1);
    }
    else
    {
        const auto users = proc.presets.getUserPresetNames();
        const int idx = id - 1001;
        if (juce::isPositiveAndBelow (idx, users.size()))
            proc.presets.loadUser (users[idx]);
    }
}

void YMYPanel::stepPreset (int delta)
{
    std::vector<int> ids;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (const int id = presetBox.getItemId (i); id > 0)
            ids.push_back (id);

    if (ids.empty())
        return;

    const int current = presetBox.getSelectedId();
    int idx = -1;
    for (size_t i = 0; i < ids.size(); ++i)
        if (ids[i] == current)
            idx = (int) i;

    if (idx < 0)
        idx = (delta > 0) ? -1 : 0;

    const int n = (int) ids.size();
    idx = ((idx + delta) % n + n) % n;
    presetBox.setSelectedId (ids[(size_t) idx], juce::sendNotification);
}

void YMYPanel::promptForName (const juce::String& title, const juce::String& initial,
                              std::function<void (juce::String)> onOk)
{
    auto* w = new juce::AlertWindow (title, "Preset name:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", initial, juce::String());
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<YMYPanel> safe (this);
    w->enterModalState (true,
                        juce::ModalCallbackFunction::create ([safe, w, onOk] (int result)
                        {
                            if (result == 1 && safe != nullptr)
                                onOk (w->getTextEditorContents ("name"));
                        }),
                        true);
}

void YMYPanel::showInfo (const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::InfoIcon)
                                      .withTitle (title)
                                      .withMessage (message)
                                      .withButton ("OK")
                                      .withAssociatedComponent (this),
                                  [] (int) {});
}

//==============================================================================
YMYDelayAudioProcessorEditor::YMYDelayAudioProcessorEditor (YMYDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    setResizable (false, false);
    addAndMakeVisible (panel);
    panel.setBounds (0, 0, YMYPanel::kBaseW, YMYPanel::kBaseH);
    panel.onScaleChanged = [this] (int index) { applyScale (index); };
    applyScale (p.getGuiScaleIndex());
}

void YMYDelayAudioProcessorEditor::applyScale (int index)
{
    const float s = ymy::params::guiScaleFromIndex (index);
    panel.setTransform (juce::AffineTransform::scale (s));
    setSize (juce::roundToInt ((float) YMYPanel::kBaseW * s), juce::roundToInt ((float) YMYPanel::kBaseH * s));
}
