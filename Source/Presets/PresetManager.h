// YMY DELAY - BY ORYMY / YMY RECORDS
// Source/Presets/PresetManager.h
// Presets de fabrica y presets de usuario (guardar, cargar, renombrar, eliminar).

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace ymy
{
struct FactoryPreset
{
    const char* name;
    const char* category;
    bool  sync;
    int   note;        // indice en divisionLabels()
    float freeMs;
    float feedback;    // %
    float mix;         // %
    float tone;        // %
    float width;       // %
    float duck;        // %
    int   mode;        // 0 mono, 1 stereo, 2 ping-pong
    bool  analog;
    bool  filter;
    float hp;          // Hz
    float lp;          // Hz
    float outputDb;
};

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    static const std::vector<FactoryPreset>& factoryPresets();

    void loadFactory (int index);

    juce::StringArray getUserPresetNames() const;
    bool saveUser   (const juce::String& name);
    bool loadUser   (const juce::String& name);
    bool renameUser (const juce::String& oldName, const juce::String& newName);
    bool deleteUser (const juce::String& name);

    juce::String getCurrentName() const;
    bool         isCurrentUser() const;

    static juce::File getUserDirectory();

private:
    void setParam (const char* id, float plainValue);
    void setCurrent (const juce::String& name, bool isUser);
    static juce::File fileFor (const juce::String& name);
    static juce::String cleanName (const juce::String& raw);

    juce::AudioProcessorValueTreeState& apvts;
};
} // namespace ymy
