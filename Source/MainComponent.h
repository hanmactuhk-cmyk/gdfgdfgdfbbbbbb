#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "AudioEngine.h"
#include "ProjectManager.h"
#include "Popups.h"
#include "GuiTheme.h"

class MainComponent : public juce::Component,
                      public juce::Timer,
                      public juce::Slider::Listener,
                      public juce::Button::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

private:
    void openPopup(const juce::String& title, juce::Component* comp, int w, int h);
    void updateButtonGlows();

    AudioEngine audioEngine;
    ProjectManager projectManager;
    HNStudioTheme::NeonLookAndFeel neonLnf;

    // Popups windows
    std::unique_ptr<NeonDialogWindow> currentPopup;

    // Top Bar
    juce::TextButton liveToggleButton { "● LIVE ON" };
    juce::TextButton audioIoButton { "AUDIO I/O" };
    juce::TextButton autoKeyButton { "AUTO KEY: C Maj" };
    juce::TextButton newProjectButton { "NEW" };
    juce::TextButton openProjectButton { "OPEN" };
    juce::TextButton saveProjectButton { "SAVE" };
    juce::TextButton guideButton { "HƯỚNG DẪN" };

    // Faders & Meters
    juce::Slider micVolSlider;
    juce::Slider musicVolSlider;
    juce::Slider masterVolSlider;

    // DSP Quick Toggles & Edit buttons
    juce::TextButton gateToggle { "GATE: ON" };
    juce::TextButton gateEditBtn { "Chỉnh" };

    juce::TextButton compToggle { "COMP: ON" };
    juce::TextButton compEditBtn { "Chỉnh" };

    juce::TextButton eqToggle { "EQ: ON" };
    juce::TextButton eqEditBtn { "Chỉnh" };

    juce::TextButton deEsserToggle { "DE-ESS: ON" };
    juce::TextButton deEsserEditBtn { "Chỉnh" };

    juce::TextButton reverbToggle { "REVERB: ON" };
    juce::TextButton reverbEditBtn { "Chỉnh" };

    juce::TextButton limiterToggle { "LIMIT: ON" };
    juce::TextButton limiterEditBtn { "Chỉnh" };

    // Auto-Tune & VST Controls
    juce::TextButton autoTuneBtn { "AUTO-TUNE" };
    juce::TextButton addVstBtn { "+ ADD VST3" };
    juce::TextButton vstRackBtn { "VST RACK" };

    // Music Player Controls
    juce::TextButton loadMusicBtn { "MỞ NHẠC BEAT" };
    juce::Label trackNameLabel;
    juce::TextButton playMusicBtn { "▶" };
    juce::TextButton pauseMusicBtn { "⏸" };
    juce::TextButton stopMusicBtn { "■" };
    juce::Slider musicTimelineSlider;
    juce::Label musicTimeLabel;

    // SFX Buttons
    juce::TextButton sfxLaughBtn { "😂 Cười" };
    juce::TextButton sfxApplauseBtn { "👏 Vỗ tay" };
    juce::TextButton sfxCrowdBtn { "🎉 Hò reo" };
    juce::TextButton sfxHornBtn { "📢 Còi hơi" };

    // Recording Controls
    juce::TextButton recordBtn { "● RECORD" };
    juce::Label recordTimeLabel;
    juce::TextButton openRecordFolderBtn { "📁 THƯ MỤC" };

    // Waveform buffers for rendering
    std::array<float, AudioEngine::WaveformBufferSize> micWaveformData{};
    std::array<float, AudioEngine::WaveformBufferSize> outWaveformData{};

    // Clip flash timer
    int clipFlashCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
