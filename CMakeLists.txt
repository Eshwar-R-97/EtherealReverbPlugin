cmake_minimum_required(VERSION 3.22)
project(EtherealReverb VERSION 1.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Allow pre-cloned JUCE to be used without re-downloading
set(FETCHCONTENT_SOURCE_DIR_JUCE "${CMAKE_BINARY_DIR}/_deps/juce-src" CACHE PATH "")

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(JUCE Catch2)

juce_add_plugin(EtherealReverb
    COMPANY_NAME                "EshwarRajasekar"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    PLUGIN_MANUFACTURER_CODE    Eshr
    PLUGIN_CODE                 Ervb
    FORMATS                     AU VST3 Standalone
    PRODUCT_NAME                "Ethereal Reverb"
    VST3_CATEGORIES             "Fx|Reverb"
    AU_MAIN_TYPE                kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED  TRUE
    MICROPHONE_PERMISSION_TEXT     "Ethereal Reverb needs microphone access to process live audio."
)

juce_generate_juce_header(EtherealReverb)

target_sources(EtherealReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSPEngine.cpp
)

target_compile_definitions(EtherealReverb
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=1
)

target_link_libraries(EtherealReverb
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── Unit tests ────────────────────────────────────────────────────────────────
juce_add_console_app(EtherealReverbTests PRODUCT_NAME "EtherealReverbTests")
juce_generate_juce_header(EtherealReverbTests)

target_sources(EtherealReverbTests
    PRIVATE
        Tests/CircularBufferTests.cpp
        Tests/PreDelayTests.cpp
        Tests/GranularShimmerTests.cpp
        Tests/SSBShimmerTests.cpp
        Tests/FDNTests.cpp
        Tests/AllPassBloomTiltTests.cpp
        Tests/ReverseReverbTests.cpp
        Source/DSPEngine.cpp
)

target_include_directories(EtherealReverbTests PRIVATE Source)

target_compile_definitions(EtherealReverbTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(EtherealReverbTests
    PRIVATE
        Catch2::Catch2WithMain
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_core
)
