// YMY DELAY - BY ORYMY / YMY RECORDS
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <memory>

#include "PluginProcessor.h"
#include "Utilities/YMYLookAndFeel.h"

// Panel con toda la interfaz en coordenadas logicas fijas (780 x 440).
// El editor aplica GUI SCALE como transformacion real sobre este panel.
class YMYPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int kBaseW = 780;
    static constexpr int kBaseH = 440;

    explicit YMYPanel (YMYDelayAudioProcessor& p);
    ~YMYPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void (int)> onScaleChanged;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob    { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };
    struct HSlider { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };

    void initKnob (Knob& k, const juce::String& text, const char* id);
    void initHSlider (HSlider& h, const juce::String& text, const char* id, int textBoxWidth);
    void initToggle (juce::ToggleButton& b, std::unique_ptr<ButtonAtt>& att, const char* id);
    void initDisplay (juce::Label& l);

    void timerCallback() override;
    void rebuildPresetMenu();
    void onPresetChosen();
    void stepPreset (int delta);
    void promptForName (const juce::String& title, const juce::String& initial,
                        std::function<void (juce::String)> onOk);
    void showInfo (const juce::String& title, const juce::String& message);

    YMYDelayAudioProcessor& proc;
    ymy::ui::YMYLookAndFeel lnf;

    Knob timeK, feedbackK, mixK, toneK, widthK, duckK;
    HSlider hpH, lpH, bpmH, outH;

    juce::ToggleButton onBtn { "ON" }, syncBtn { "SYNC" }, analogBtn { "ANALOG" }, filterBtn { "FILTER" };
    juce::TextButton tapBtn { "TAP" }, prevBtn { "<" }, nextBtn { ">" },
                     saveBtn { "SAVE" }, renameBtn { "RENAME" }, deleteBtn { "DELETE" };
    juce::ComboBox noteBox, modeBox, scaleBox, presetBox;
    juce::Label bpmLabel, timeLabel;

    std::unique_ptr<ButtonAtt> onAtt, syncAtt, analogAtt, filterAtt;
    std::unique_ptr<ComboAtt> noteAtt, modeAtt;

    float meterL = 0.0f, meterR = 0.0f;
    juce::String lastBpmText, lastTimeText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YMYPanel)
};

//==============================================================================
class YMYDelayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit YMYDelayAudioProcessorEditor (YMYDelayAudioProcessor&);
    ~YMYDelayAudioProcessorEditor() override = default;

private:
    void applyScale (int index);

    YMYPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YMYDelayAudioProcessorEditor)
};
