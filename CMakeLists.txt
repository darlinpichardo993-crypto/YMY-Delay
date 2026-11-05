cmake_minimum_required(VERSION 3.22)

# Runtime estatico en Windows: el usuario no necesita instalar el VC++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(YMY_DELAY VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE fijado a una version concreta para que la compilacion sea reproducible.
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(juce)

juce_add_plugin(YMYDelay
    COMPANY_NAME                "ORYMY"
    COMPANY_WEBSITE             "https://github.com/"
    BUNDLE_ID                   "com.orymy.ymydelay"
    PRODUCT_NAME                "YMY DELAY"
    DESCRIPTION                 "YMY DELAY - Vintage Bachata Delay by ORYMY (YMY RECORDS)"
    PLUGIN_MANUFACTURER_CODE    Ymyr
    PLUGIN_CODE                 Ymyd
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Fx Delay
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(YMYDelay PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets/PresetManager.cpp)

target_include_directories(YMYDelay PRIVATE Source)

target_compile_definitions(YMYDelay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_compile_options(YMYDelay PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(YMYDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
