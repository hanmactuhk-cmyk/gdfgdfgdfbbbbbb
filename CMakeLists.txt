cmake_minimum_required(VERSION 3.22)
project(HNStudioMusikAI VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer a self-contained MSVC runtime for the Windows release package.
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (MSVC)
    add_compile_options(/MP /utf-8 /W3)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(HNStudioMusikAI
    PRODUCT_NAME "HNStudio Musik AI"
    VERSION 1.0.0
    COMPANY_NAME "Hoai Nguyen Studio"
    BUNDLE_IDENTIFIER "com.hoainguyenstudio.hnstudiomusikai"
)

target_sources(HNStudioMusikAI PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/AudioEngine.h
    Source/AudioEngine.cpp
    Source/DspChain.h
    Source/DspChain.cpp
    Source/NoiseGate.h
    Source/Compressor.h
    Source/ParametricEQ.h
    Source/DeEsser.h
    Source/ReverbProcessor.h
    Source/LimiterProcessor.h
    Source/VstRack.h
    Source/VstRack.cpp
    Source/AutoKeyDetector.h
    Source/AutoKeyDetector.cpp
    Source/MusicPlayer.h
    Source/MusicPlayer.cpp
    Source/SfxPlayer.h
    Source/AudioRecorder.h
    Source/ProjectManager.h
    Source/ProjectManager.cpp
    Source/GuiTheme.h
    Source/Popups.h
    Source/Popups.cpp
)

target_include_directories(HNStudioMusikAI PRIVATE Source)

target_compile_definitions(HNStudioMusikAI PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=0
    JUCE_CUSTOM_VST3_SDK=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(HNStudioMusikAI PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)
