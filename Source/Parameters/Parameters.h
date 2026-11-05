// YMY DELAY - BY ORYMY / YMY RECORDS
// Source/Parameters/Parameters.h
// Identificadores, rangos y layout de todos los parametros automatizables.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>
#include <vector>

#include "DSP/TempoSync.h"

namespace ymy::params
{
inline constexpr const char* kOn        = "on";
inline constexpr const char* kSync      = "sync";
inline constexpr const char* kNote      = "note";
inline constexpr const char* kFreeMs    = "free_ms";
inline constexpr const char* kManualBpm = "manual_bpm";
inline constexpr const char* kFeedback  = "feedback";
inline constexpr const char* kMix       = "mix";
inline constexpr const char* kTone      = "tone";
inline constexpr const char* kWidth     = "width";
inline constexpr const char* kDuck      = "duck";
inline constexpr const char* kMode      = "mode";
inline constexpr const char* kAnalog    = "analog";
inline constexpr const char* kFilter    = "filter";
inline constexpr const char* kHp        = "hp";
inline constexpr const char* kLp        = "lp";
inline constexpr const char* kOutput    = "output";

// Parametros que guardan los presets (on y manual_bpm no forman parte de un preset).
inline constexpr std::array<const char*, 14> kPresetParams {
    kSync, kNote, kFreeMs, kFeedback, kMix, kTone, kWidth,
    kDuck, kMode, kAnalog, kFilter, kHp, kLp, kOutput
};

inline const juce::StringArray& modeLabels()
{
    static const juce::StringArray labels { "MONO", "STEREO", "PING-PONG" };
    return labels;
}

inline const juce::StringArray& guiScaleLabels()
{
    static const juce::StringArray labels { "75%", "90%", "100%", "110%", "125%", "150%", "175%", "200%" };
    return labels;
}

inline constexpr int kDefaultGuiScaleIndex = 2; // 100 %

inline float guiScaleFromIndex (int index) noexcept
{
    static constexpr float scales[] = { 0.75f, 0.90f, 1.00f, 1.10f, 1.25f, 1.50f, 1.75f, 2.00f };
    return scales[juce::jlimit (0, 7, index)];
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;

    auto f0 = [] (float v, int) { return String (roundToInt (v)); };
    auto f1 = [] (float v, int) { return String (v, 1); };

    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { kOn, 1 },   "On",   true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { kSync, 1 }, "Sync", true));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { kNote, 1 }, "Note",
                                                         dsp::divisionLabels(), 7)); // 1/8

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kFreeMs, 1 }, "Time (Free)",
                     NormalisableRange<float> (1.0f, 500.0f, 0.01f, 0.45f), 120.0f,
                     AudioParameterFloatAttributes().withLabel (" ms").withStringFromValueFunction (f1)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kManualBpm, 1 }, "Manual BPM",
                     NormalisableRange<float> (20.0f, 300.0f, 0.01f), 120.0f,
                     AudioParameterFloatAttributes().withLabel (" BPM").withStringFromValueFunction (f1)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kFeedback, 1 }, "Feedback",
                     NormalisableRange<float> (0.0f, 90.0f, 0.1f), 25.0f,
                     AudioParameterFloatAttributes().withLabel (" %").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kMix, 1 }, "Mix",
                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f,
                     AudioParameterFloatAttributes().withLabel (" %").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kTone, 1 }, "Tone",
                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
                     AudioParameterFloatAttributes().withLabel (" %").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kWidth, 1 }, "Width",
                     NormalisableRange<float> (0.0f, 200.0f, 0.1f), 100.0f,
                     AudioParameterFloatAttributes().withLabel (" %").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kDuck, 1 }, "Duck",
                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
                     AudioParameterFloatAttributes().withLabel (" %").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { kMode, 1 }, "Mode", modeLabels(), 1));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { kAnalog, 1 }, "Analog", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { kFilter, 1 }, "Filter", true));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kHp, 1 }, "High-Pass",
                     NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.4f), 120.0f,
                     AudioParameterFloatAttributes().withLabel (" Hz").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kLp, 1 }, "Low-Pass",
                     NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.4f), 7000.0f,
                     AudioParameterFloatAttributes().withLabel (" Hz").withStringFromValueFunction (f0)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { kOutput, 1 }, "Output",
                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                     AudioParameterFloatAttributes().withLabel (" dB").withStringFromValueFunction (f1)));

    return { std::make_move_iterator (p.begin()), std::make_move_iterator (p.end()) };
}
} // namespace ymy::params
