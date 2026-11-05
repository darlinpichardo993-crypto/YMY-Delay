#include "Presets/PresetManager.h"

#include "Parameters/Parameters.h"

namespace ymy
{
namespace
{
// Indices de NOTE: 0=1/1 1=1/2 2=1/2D 3=1/2T 4=1/4 5=1/4D 6=1/4T 7=1/8 8=1/8D 9=1/8T
//                  10=1/16 11=1/16D 12=1/16T 13=1/32 14=1/32D 15=1/32T
// Modo: 0 = MONO, 1 = STEREO, 2 = PING-PONG
const std::vector<FactoryPreset> kFactory {
    // name, category, sync, note, freeMs, fb, mix, tone, width, duck, mode, analog, filter, hp, lp, out
    { "BACHATA 90s",       "BACHATA", true,  7, 120.f, 22.f, 22.f, 40.f,  90.f, 35.f, 0, true,  true, 150.f, 5000.f, 0.f },
    { "BACHATA 2000s",     "BACHATA", true, 11, 120.f, 28.f, 24.f, 52.f, 110.f, 30.f, 1, true,  true, 140.f, 6500.f, 0.f },
    { "ROMANTIC BACHATA",  "BACHATA", true,  8, 120.f, 32.f, 26.f, 45.f, 120.f, 40.f, 1, true,  true, 160.f, 5500.f, 0.f },
    { "CLASSIC BACHATA",   "BACHATA", true, 10, 120.f, 18.f, 20.f, 35.f,  80.f, 35.f, 0, true,  true, 180.f, 4800.f, 0.f },
    { "MODERN BACHATA",    "BACHATA", true,  7, 120.f, 20.f, 20.f, 62.f, 130.f, 45.f, 1, false, true, 120.f, 8500.f, 0.f },

    { "LEAD VOCAL SHORT",  "VOCALS",  true, 10, 120.f, 15.f, 15.f, 50.f, 100.f, 55.f, 0, false, true, 200.f, 7000.f, 0.f },
    { "ROMANTIC VOCAL",    "VOCALS",  true,  8, 120.f, 30.f, 24.f, 42.f, 115.f, 45.f, 1, true,  true, 150.f, 5800.f, 0.f },
    { "SECOND VOICE",      "VOCALS",  true, 11, 120.f, 24.f, 30.f, 48.f, 125.f, 25.f, 1, true,  true, 170.f, 6000.f, 0.f },
    { "BACKING VOCALS",    "VOCALS",  true,  7, 120.f, 35.f, 32.f, 38.f, 150.f, 20.f, 1, true,  true, 220.f, 5200.f, 0.f },
    { "ADLIBS",            "VOCALS",  true,  9, 120.f, 42.f, 35.f, 55.f, 140.f, 30.f, 2, false, true, 200.f, 7500.f, 0.f },
    { "VOCAL ENDINGS",     "VOCALS",  true,  4, 120.f, 45.f, 30.f, 40.f, 120.f, 70.f, 1, true,  true, 180.f, 5200.f, 0.f },
    { "SOFT SLAP",         "VOCALS",  false, 7,  85.f,  5.f, 18.f, 45.f, 100.f, 40.f, 0, true,  true, 160.f, 6000.f, 0.f },
    { "DARK VOCAL",        "VOCALS",  true,  8, 120.f, 28.f, 24.f, 15.f, 100.f, 40.f, 0, true,  true, 200.f, 3800.f, 0.f },
    { "WARM VOCAL",        "VOCALS",  true, 10, 120.f, 20.f, 20.f, 30.f, 105.f, 40.f, 1, true,  true, 150.f, 5000.f, 0.f },

    { "ANALOG ECHO",       "SPECIAL", true,  7, 120.f, 38.f, 28.f, 35.f, 100.f, 15.f, 1, true,  true, 120.f, 4500.f, 0.f },
    { "STEREO ROMANCE",    "SPECIAL", true,  8, 120.f, 30.f, 26.f, 45.f, 160.f, 40.f, 1, true,  true, 160.f, 6000.f, 0.f },
    { "SUBTLE DELAY",      "SPECIAL", true, 10, 120.f, 10.f, 12.f, 45.f, 100.f, 60.f, 0, false, true, 220.f, 6500.f, 0.f },
    { "DARK VINTAGE",      "SPECIAL", true,  7, 120.f, 40.f, 26.f, 10.f, 110.f, 30.f, 1, true,  true, 140.f, 3200.f, 0.f },
    { "WIDE BACHATA",      "SPECIAL", true, 11, 120.f, 30.f, 28.f, 55.f, 190.f, 35.f, 2, true,  true, 160.f, 6500.f, 0.f },
    { "GUITAR ECHO",       "SPECIAL", true,  8, 120.f, 36.f, 30.f, 50.f, 140.f,  0.f, 1, true,  true, 120.f, 7500.f, 0.f },
};

constexpr const char* kExt = ".ymypreset";
} // namespace

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state) {}

const std::vector<FactoryPreset>& PresetManager::factoryPresets() { return kFactory; }

void PresetManager::setParam (const char* id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void PresetManager::setCurrent (const juce::String& name, bool isUser)
{
    apvts.state.setProperty ("presetName", name, nullptr);
    apvts.state.setProperty ("presetIsUser", isUser, nullptr);
}

juce::String PresetManager::getCurrentName() const { return apvts.state.getProperty ("presetName", "").toString(); }
bool PresetManager::isCurrentUser() const          { return (bool) apvts.state.getProperty ("presetIsUser", false); }

void PresetManager::loadFactory (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) kFactory.size()))
        return;

    const auto& f = kFactory[(size_t) index];

    setParam (params::kSync,     f.sync ? 1.0f : 0.0f);
    setParam (params::kNote,     (float) f.note);
    setParam (params::kFreeMs,   f.freeMs);
    setParam (params::kFeedback, f.feedback);
    setParam (params::kMix,      f.mix);
    setParam (params::kTone,     f.tone);
    setParam (params::kWidth,    f.width);
    setParam (params::kDuck,     f.duck);
    setParam (params::kMode,     (float) f.mode);
    setParam (params::kAnalog,   f.analog ? 1.0f : 0.0f);
    setParam (params::kFilter,   f.filter ? 1.0f : 0.0f);
    setParam (params::kHp,       f.hp);
    setParam (params::kLp,       f.lp);
    setParam (params::kOutput,   f.outputDb);

    setCurrent (f.name, false);
}

//==============================================================================
juce::File PresetManager::getUserDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("ORYMY").getChildFile ("YMY DELAY").getChildFile ("Presets");
}

juce::String PresetManager::cleanName (const juce::String& raw)
{
    return juce::File::createLegalFileName (raw.trim()).trim();
}

juce::File PresetManager::fileFor (const juce::String& name)
{
    return getUserDirectory().getChildFile (name + kExt);
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getUserDirectory().findChildFiles (juce::File::findFiles, false, juce::String ("*") + kExt))
        names.add (f.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool PresetManager::saveUser (const juce::String& rawName)
{
    const auto name = cleanName (rawName);
    if (name.isEmpty())
        return false;

    if (! getUserDirectory().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("YMYPreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("version", 1);

    for (auto* id : params::kPresetParams)
    {
        if (auto* v = apvts.getRawParameterValue (id))
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", juce::String (id));
            e->setAttribute ("value", (double) v->load());
        }
    }

    if (! xml.writeTo (fileFor (name)))
        return false;

    setCurrent (name, true);
    return true;
}

bool PresetManager::loadUser (const juce::String& name)
{
    const auto file = fileFor (name);
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("YMYPreset"))
        return false;

    for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
    {
        const auto id = e->getStringAttribute ("id");
        if (id.isNotEmpty())
            setParam (id.toRawUTF8(), (float) e->getDoubleAttribute ("value"));
    }

    setCurrent (name, true);
    return true;
}

bool PresetManager::renameUser (const juce::String& oldName, const juce::String& rawNew)
{
    const auto newName = cleanName (rawNew);
    const auto src = fileFor (oldName);
    const auto dst = fileFor (newName);

    if (newName.isEmpty() || ! src.existsAsFile() || dst.existsAsFile())
        return false;

    if (! src.moveFileTo (dst))
        return false;

    if (getCurrentName() == oldName && isCurrentUser())
        setCurrent (newName, true);

    return true;
}

bool PresetManager::deleteUser (const juce::String& name)
{
    const auto file = fileFor (name);
    if (! file.existsAsFile() || ! file.deleteFile())
        return false;

    if (getCurrentName() == name && isCurrentUser())
        setCurrent ({}, false);

    return true;
}
} // namespace ymy
