#include "MainComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

MainComponent::MainComponent()
    : projectManager(audioEngine)
{
    setLookAndFeel(&neonLnf);
    setSize(1080, 780);

    // Initialize Audio Engine
    audioEngine.initAudioDevice();

    // Top Bar Buttons
    auto setupTopBtn = [this](juce::TextButton& btn, const juce::Colour& textCol, const juce::Colour& bgCol)
    {
        addAndMakeVisible(btn);
        btn.setColour(juce::TextButton::textColourOffId, textCol);
        btn.setColour(juce::TextButton::buttonColourId, bgCol);
        btn.addListener(this);
    };

    setupTopBtn(liveToggleButton, HNStudioTheme::neonGreen, HNStudioTheme::neonGreen.withAlpha(0.2f));
    setupTopBtn(audioIoButton, HNStudioTheme::neonCyan, HNStudioTheme::bgCardHighlight);
    setupTopBtn(autoKeyButton, HNStudioTheme::neonMagenta, HNStudioTheme::bgCardHighlight);
    setupTopBtn(newProjectButton, HNStudioTheme::textBright, HNStudioTheme::bgCard);
    setupTopBtn(openProjectButton, HNStudioTheme::textBright, HNStudioTheme::bgCard);
    setupTopBtn(saveProjectButton, HNStudioTheme::textBright, HNStudioTheme::bgCard);
    setupTopBtn(guideButton, HNStudioTheme::neonAmber, HNStudioTheme::bgCardHighlight);

    // Faders
    auto setupFader = [this](juce::Slider& s, float defaultVal)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        s.setRange(0.0, 1.5, 0.01);
        s.setValue(defaultVal, juce::dontSendNotification);
        s.addListener(this);
    };

    setupFader(micVolSlider, 1.0f);
    setupFader(musicVolSlider, 0.85f);
    setupFader(masterVolSlider, 1.0f);

    // DSP Quick Buttons
    auto setupDspBtnPair = [this](juce::TextButton& toggle, juce::TextButton& edit)
    {
        addAndMakeVisible(toggle);
        addAndMakeVisible(edit);
        toggle.setColour(juce::TextButton::textColourOffId, HNStudioTheme::neonCyan);
        toggle.setColour(juce::TextButton::buttonColourId, HNStudioTheme::bgCard);
        edit.setColour(juce::TextButton::textColourOffId, HNStudioTheme::textSecondary);
        edit.setColour(juce::TextButton::buttonColourId, HNStudioTheme::bgCardHighlight);
        toggle.addListener(this);
        edit.addListener(this);
    };

    setupDspBtnPair(gateToggle, gateEditBtn);
    setupDspBtnPair(compToggle, compEditBtn);
    setupDspBtnPair(eqToggle, eqEditBtn);
    setupDspBtnPair(deEsserToggle, deEsserEditBtn);
    setupDspBtnPair(reverbToggle, reverbEditBtn);
    setupDspBtnPair(limiterToggle, limiterEditBtn);

    // Auto-Tune & VST Buttons
    setupTopBtn(autoTuneBtn, HNStudioTheme::neonMagenta, HNStudioTheme::neonMagenta.withAlpha(0.2f));
    setupTopBtn(addVstBtn, HNStudioTheme::neonCyan, HNStudioTheme::neonCyan.withAlpha(0.2f));
    setupTopBtn(vstRackBtn, HNStudioTheme::neonPurple, HNStudioTheme::bgCardHighlight);

    // Music Player
    setupTopBtn(loadMusicBtn, HNStudioTheme::neonCyan, HNStudioTheme::bgCardHighlight);
    setupTopBtn(playMusicBtn, HNStudioTheme::neonGreen, HNStudioTheme::bgCard);
    setupTopBtn(pauseMusicBtn, HNStudioTheme::neonAmber, HNStudioTheme::bgCard);
    setupTopBtn(stopMusicBtn, HNStudioTheme::neonRed, HNStudioTheme::bgCard);

    addAndMakeVisible(trackNameLabel);
    trackNameLabel.setText("Chưa chọn bài hát (Hỗ trợ MP3, WAV, FLAC, OGG)", juce::dontSendNotification);
    trackNameLabel.setColour(juce::Label::textColourId, HNStudioTheme::textDim);

    addAndMakeVisible(musicTimelineSlider);
    musicTimelineSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    musicTimelineSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    musicTimelineSlider.setRange(0.0, 100.0, 0.1);
    musicTimelineSlider.addListener(this);

    addAndMakeVisible(musicTimeLabel);
    musicTimeLabel.setText("00:00 / 00:00", juce::dontSendNotification);
    musicTimeLabel.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);

    // SFX Buttons
    setupTopBtn(sfxLaughBtn, HNStudioTheme::neonAmber, HNStudioTheme::bgCard);
    setupTopBtn(sfxApplauseBtn, HNStudioTheme::neonCyan, HNStudioTheme::bgCard);
    setupTopBtn(sfxCrowdBtn, HNStudioTheme::neonPurple, HNStudioTheme::bgCard);
    setupTopBtn(sfxHornBtn, HNStudioTheme::neonRed, HNStudioTheme::bgCard);

    // Recording
    setupTopBtn(recordBtn, HNStudioTheme::neonRed, HNStudioTheme::neonRed.withAlpha(0.25f));
    setupTopBtn(openRecordFolderBtn, HNStudioTheme::textBright, HNStudioTheme::bgCard);

    addAndMakeVisible(recordTimeLabel);
    recordTimeLabel.setText("00:00 | 24-bit WAV", juce::dontSendNotification);
    recordTimeLabel.setColour(juce::Label::textColourId, HNStudioTheme::neonRed);
    recordTimeLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    startTimerHz(30); // 30 FPS UI refresh
}

MainComponent::~MainComponent()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgDark);

    // Header Background
    g.setColour(HNStudioTheme::bgPanel);
    g.fillRect(0, 0, getWidth(), 65);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawLine(0, 65, (float)getWidth(), 65, 1.0f);

    // Logo & Brand Text
    g.setColour(HNStudioTheme::neonCyan);
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("✦ HNSTUDIO MUSIK AI ✦", 20, 10, 320, 26, juce::Justification::left);

    g.setFont(12.0f);
    g.setColour(HNStudioTheme::textSecondary);
    g.drawText("Hát Live Chuyên Nghiệp | Hoài Nguyễn Studio (Zalo: 0965.043.000)", 22, 36, 450, 20, juce::Justification::left);

    // =========================================================================
    // CHANNEL STRIPS (MIC & MUSIC)
    // =========================================================================
    int stripTop = 115;

    // MIC Box
    g.setColour(HNStudioTheme::bgPanel);
    g.fillRoundedRectangle(20.0f, (float)stripTop, 110.0f, 220.0f, 6.0f);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawRoundedRectangle(20.0f, (float)stripTop, 110.0f, 220.0f, 6.0f, 1.0f);

    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.setColour(HNStudioTheme::neonCyan);
    g.drawText("MIC IN", 20, stripTop + 6, 110, 18, juce::Justification::centred);

    // Mic Level Meter
    int micMeterX = 88, micMeterY = stripTop + 30, micMeterW = 12, micMeterH = 150;
    g.setColour(HNStudioTheme::bgCardHighlight);
    g.fillRect(micMeterX, micMeterY, micMeterW, micMeterH);

    float micPeak = audioEngine.getMicLevelPeak();
    int micFillH = (int)(juce::jlimit(0.0f, 1.0f, micPeak) * micMeterH);
    if (micFillH > 0)
    {
        g.setColour(micPeak > 0.95f ? HNStudioTheme::neonRed : (micPeak > 0.7f ? HNStudioTheme::neonAmber : HNStudioTheme::neonGreen));
        g.fillRect(micMeterX, micMeterY + micMeterH - micFillH, micMeterW, micFillH);
    }

    // MUSIC Box
    int musicX = 145;
    g.setColour(HNStudioTheme::bgPanel);
    g.fillRoundedRectangle((float)musicX, (float)stripTop, 110.0f, 220.0f, 6.0f);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawRoundedRectangle((float)musicX, (float)stripTop, 110.0f, 220.0f, 6.0f, 1.0f);

    g.setColour(HNStudioTheme::neonMagenta);
    g.drawText("MUSIC", musicX, stripTop + 6, 110, 18, juce::Justification::centred);

    // Music Level Meter
    int musicMeterX = musicX + 68, musicMeterY = stripTop + 30, musicMeterW = 12, musicMeterH = 150;
    g.setColour(HNStudioTheme::bgCardHighlight);
    g.fillRect(musicMeterX, musicMeterY, musicMeterW, musicMeterH);

    float musicVol = audioEngine.getMusicVolume();
    int musicFillH = audioEngine.getMusicPlayer().isPlaying() ? (int)(juce::jlimit(0.0f, 1.0f, musicVol * 0.7f) * musicMeterH) : 0;
    if (musicFillH > 0)
    {
        g.setColour(HNStudioTheme::neonMagenta);
        g.fillRect(musicMeterX, musicMeterY + musicMeterH - musicFillH, musicMeterW, musicFillH);
    }

    // =========================================================================
    // REALTIME LIVE WAVEFORM DISPLAY
    // =========================================================================
    int waveX = 275, waveY = stripTop, waveW = getWidth() - 410, waveH = 220;
    g.setColour(HNStudioTheme::bgPanel);
    g.fillRoundedRectangle((float)waveX, (float)waveY, (float)waveW, (float)waveH, 6.0f);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawRoundedRectangle((float)waveX, (float)waveY, (float)waveW, (float)waveH, 6.0f, 1.0f);

    g.setFont(12.0f);
    g.setColour(HNStudioTheme::neonCyan);
    g.drawText("REALTIME OSCILLOSCOPE (MIC & OUTPUT)", waveX + 15, waveY + 8, 300, 18, juce::Justification::left);

    // Grid center line
    float midY = (float)waveY + (float)waveH * 0.5f;
    g.setColour(HNStudioTheme::borderSubtle.withAlpha(0.6f));
    g.drawDashedLine(juce::Line<float>((float)waveX, midY, (float)(waveX + waveW), midY), nullptr, 0, 1.0f);

    // Draw Output Waveform Path (Background glow in Magenta)
    audioEngine.getOutputWaveform(outWaveformData);
    juce::Path outPath;
    outPath.startNewSubPath((float)waveX, midY);
    for (int i = 0; i < AudioEngine::WaveformBufferSize; ++i)
    {
        float x = (float)waveX + ((float)i / (float)AudioEngine::WaveformBufferSize) * (float)waveW;
        float y = midY - outWaveformData[i] * ((float)waveH * 0.42f);
        outPath.lineTo(x, y);
    }
    g.setColour(HNStudioTheme::neonMagenta.withAlpha(0.5f));
    g.strokePath(outPath, juce::PathStrokeType(1.5f));

    // Draw Mic Waveform Path (Foreground bright Cyan)
    audioEngine.getMicWaveform(micWaveformData);
    juce::Path micPath;
    micPath.startNewSubPath((float)waveX, midY);
    for (int i = 0; i < AudioEngine::WaveformBufferSize; ++i)
    {
        float x = (float)waveX + ((float)i / (float)AudioEngine::WaveformBufferSize) * (float)waveW;
        float y = midY - micWaveformData[i] * ((float)waveH * 0.42f);
        micPath.lineTo(x, y);
    }
    g.setColour(HNStudioTheme::neonCyan);
    g.strokePath(micPath, juce::PathStrokeType(2.0f));

    // =========================================================================
    // MASTER CHANNEL STRIP & CLIPPING INDICATOR
    // =========================================================================
    int masterX = getWidth() - 120, masterY = stripTop, masterW = 100, masterH = 220;
    g.setColour(HNStudioTheme::bgPanel);
    g.fillRoundedRectangle((float)masterX, (float)masterY, (float)masterW, (float)masterH, 6.0f);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawRoundedRectangle((float)masterX, (float)masterY, (float)masterW, (float)masterH, 6.0f, 1.0f);

    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.setColour(HNStudioTheme::neonGreen);
    g.drawText("MASTER", masterX, masterY + 6, masterW, 18, juce::Justification::centred);

    // CLIP Warning LED
    bool isClip = (clipFlashCounter > 0);
    g.setColour(isClip ? HNStudioTheme::neonRed : HNStudioTheme::bgCardHighlight);
    g.fillRoundedRectangle((float)(masterX + 30), (float)(masterY + 28), 40.0f, 16.0f, 3.0f);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(isClip ? HNStudioTheme::textBright : HNStudioTheme::textDim);
    g.drawText("CLIP", masterX + 30, masterY + 28, 40, 16, juce::Justification::centred);

    // Master Meter
    int masterMeterX = masterX + 68, masterMeterY = masterY + 48, masterMeterW = 12, masterMeterH = 132;
    g.setColour(HNStudioTheme::bgCardHighlight);
    g.fillRect(masterMeterX, masterMeterY, masterMeterW, masterMeterH);

    float outPeak = audioEngine.getOutputLevelPeak();
    int outFillH = (int)(juce::jlimit(0.0f, 1.0f, outPeak) * masterMeterH);
    if (outFillH > 0)
    {
        g.setColour(outPeak > 0.98f ? HNStudioTheme::neonRed : (outPeak > 0.7f ? HNStudioTheme::neonAmber : HNStudioTheme::neonGreen));
        g.fillRect(masterMeterX, masterMeterY + masterMeterH - outFillH, masterMeterW, outFillH);
    }

    // =========================================================================
    // FOOTER
    // =========================================================================
    g.setColour(HNStudioTheme::bgPanel);
    g.fillRect(0, getHeight() - 32, getWidth(), 32);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawLine(0, (float)(getHeight() - 32), (float)getWidth(), (float)(getHeight() - 32), 1.0f);

    g.setFont(12.0f);
    g.setColour(HNStudioTheme::textDim);
    g.drawText("Bản quyền © Hoài Nguyễn Studio - Zalo: 0965.043.000 | 64-bit Realtime VST3 Host Engine for Windows",
               20, getHeight() - 28, getWidth() - 40, 24, juce::Justification::centred);
}

void MainComponent::resized()
{
    // Top Bar
    int topY = 75;
    liveToggleButton.setBounds(20, topY, 110, 30);
    audioIoButton.setBounds(140, topY, 100, 30);
    autoKeyButton.setBounds(250, topY, 160, 30);
    newProjectButton.setBounds(420, topY, 65, 30);
    openProjectButton.setBounds(495, topY, 65, 30);
    saveProjectButton.setBounds(570, topY, 65, 30);
    guideButton.setBounds(getWidth() - 130, topY, 110, 30);

    // Channel Faders
    micVolSlider.setBounds(30, 145, 50, 150);
    musicVolSlider.setBounds(155, 145, 50, 150);
    masterVolSlider.setBounds(getWidth() - 110, 165, 50, 130);

    // DSP Bar
    int dspY = 350;
    int dspW = 160;
    int spacing = (getWidth() - 40 - dspW * 6) / 5 + dspW;

    auto layoutDsp = [this, dspY](juce::TextButton& toggle, juce::TextButton& edit, int x)
    {
        toggle.setBounds(x, dspY, 95, 30);
        edit.setBounds(x + 100, dspY, 55, 30);
    };

    layoutDsp(gateToggle, gateEditBtn, 20);
    layoutDsp(compToggle, compEditBtn, 20 + spacing);
    layoutDsp(eqToggle, eqEditBtn, 20 + spacing * 2);
    layoutDsp(deEsserToggle, deEsserEditBtn, 20 + spacing * 3);
    layoutDsp(reverbToggle, reverbEditBtn, 20 + spacing * 4);
    layoutDsp(limiterToggle, limiterEditBtn, 20 + spacing * 5);

    // Auto-Tune & VST Section
    int vstY = 395;
    autoTuneBtn.setBounds(20, vstY, 150, 34);
    addVstBtn.setBounds(180, vstY, 140, 34);
    vstRackBtn.setBounds(330, vstY, 130, 34);

    // Music Player Section
    int playerY = 445;
    loadMusicBtn.setBounds(20, playerY, 140, 32);
    playMusicBtn.setBounds(170, playerY, 40, 32);
    pauseMusicBtn.setBounds(215, playerY, 40, 32);
    stopMusicBtn.setBounds(260, playerY, 40, 32);
    trackNameLabel.setBounds(310, playerY, getWidth() - 450, 32);

    musicTimelineSlider.setBounds(20, playerY + 38, getWidth() - 170, 24);
    musicTimeLabel.setBounds(getWidth() - 140, playerY + 38, 120, 24);

    // SFX Soundboard
    int sfxY = 525;
    int sfxW = 130;
    sfxLaughBtn.setBounds(20, sfxY, sfxW, 34);
    sfxApplauseBtn.setBounds(20 + sfxW + 15, sfxY, sfxW, 34);
    sfxCrowdBtn.setBounds(20 + (sfxW + 15) * 2, sfxY, sfxW, 34);
    sfxHornBtn.setBounds(20 + (sfxW + 15) * 3, sfxY, sfxW, 34);

    // Recording Section
    int recY = 575;
    recordBtn.setBounds(20, recY, 150, 40);
    recordTimeLabel.setBounds(185, recY, 200, 40);
    openRecordFolderBtn.setBounds(400, recY, 130, 40);
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &micVolSlider)
        audioEngine.setMicVolume((float)micVolSlider.getValue());
    else if (slider == &musicVolSlider)
        audioEngine.setMusicVolume((float)musicVolSlider.getValue());
    else if (slider == &masterVolSlider)
        audioEngine.setMasterVolume((float)masterVolSlider.getValue());
    else if (slider == &musicTimelineSlider)
    {
        double pos = (musicTimelineSlider.getValue() / 100.0) * audioEngine.getMusicPlayer().getDuration();
        audioEngine.getMusicPlayer().setPosition(pos);
    }
}

void MainComponent::buttonClicked(juce::Button* button)
{
    auto& dsp = audioEngine.getDspChain();

    if (button == &liveToggleButton)
    {
        bool newState = !audioEngine.isLiveEnabled();
        audioEngine.setLiveEnabled(newState);
        liveToggleButton.setButtonText(newState ? "● LIVE ON" : "○ LIVE OFF");
        liveToggleButton.setColour(juce::TextButton::textColourOffId, newState ? HNStudioTheme::neonGreen : HNStudioTheme::textDim);
    }
    else if (button == &audioIoButton)
    {
        openPopup("AUDIO INPUT / OUTPUT", new AudioIoComponent(audioEngine), 600, 450);
    }
    else if (button == &autoKeyButton || button == &autoTuneBtn)
    {
        openPopup("AUTO-TUNE & AUTO KEY", new AutoTuneComponent(audioEngine), 500, 260);
    }
    else if (button == &newProjectButton)
    {
        projectManager.newProject();
        updateButtonGlows();
    }
    else if (button == &openProjectButton)
    {
        auto fc = std::make_shared<juce::FileChooser>("Mở Project HNStudio", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.hnstudio");
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, fc](const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                juce::StringArray missing;
                projectManager.loadProject(file, missing);
                updateButtonGlows();
                if (!missing.isEmpty())
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Thiếu Plugin VST",
                        "Các plugin sau không tìm thấy trên máy này:\n" + missing.joinIntoString("\n")
                    );
                }
            }
        });
    }
    else if (button == &saveProjectButton)
    {
        auto fc = std::make_shared<juce::FileChooser>("Lưu Project HNStudio", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.hnstudio");
        fc->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this, fc](const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file != juce::File())
            {
                if (!file.hasFileExtension(".hnstudio"))
                    file = file.withFileExtension(".hnstudio");
                projectManager.saveProject(file);
            }
        });
    }
    else if (button == &guideButton)
    {
        openPopup("HƯỚNG DẪN SỬ DỤNG - HOÀI NGUYỄN STUDIO", new UserGuideComponent(), 700, 500);
    }
    // DSP Toggles & Edits
    else if (button == &gateToggle)
    {
        dsp.getNoiseGate().setEnabled(!dsp.getNoiseGate().isEnabled());
        updateButtonGlows();
    }
    else if (button == &gateEditBtn)
    {
        openPopup("NOISE GATE", new NoiseGateComponent(audioEngine), 480, 200);
    }
    else if (button == &compToggle)
    {
        dsp.getCompressor().setEnabled(!dsp.getCompressor().isEnabled());
        updateButtonGlows();
    }
    else if (button == &compEditBtn)
    {
        openPopup("VOCAL COMPRESSOR", new CompressorComponent(audioEngine), 520, 230);
    }
    else if (button == &eqToggle)
    {
        dsp.getEQ().setEnabled(!dsp.getEQ().isEnabled());
        updateButtonGlows();
    }
    else if (button == &eqEditBtn)
    {
        openPopup("13-BAND PARAMETRIC EQ", new EQComponent(audioEngine), 780, 250);
    }
    else if (button == &deEsserToggle)
    {
        dsp.getDeEsser().setEnabled(!dsp.getDeEsser().isEnabled());
        updateButtonGlows();
    }
    else if (button == &deEsserEditBtn)
    {
        openPopup("DE-ESSER", new DeEsserComponent(audioEngine), 440, 200);
    }
    else if (button == &reverbToggle)
    {
        dsp.getReverb().setEnabled(!dsp.getReverb().isEnabled());
        updateButtonGlows();
    }
    else if (button == &reverbEditBtn)
    {
        openPopup("STUDIO REVERB", new ReverbComponent(audioEngine), 640, 220);
    }
    else if (button == &limiterToggle)
    {
        dsp.getLimiter().setEnabled(!dsp.getLimiter().isEnabled());
        updateButtonGlows();
    }
    else if (button == &limiterEditBtn)
    {
        openPopup("MASTER LIMITER", new LimiterComponent(audioEngine), 440, 200);
    }
    // VST Rack
    else if (button == &addVstBtn || button == &vstRackBtn)
    {
        openPopup("QUẢN LÝ VST3 INSERT RACK", new VstManagerComponent(audioEngine), 650, 400);
    }
    // Music Player
    else if (button == &loadMusicBtn)
    {
        auto fc = std::make_shared<juce::FileChooser>("Chọn file nhạc nền (Beat)", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "*.mp3;*.wav;*.flac;*.ogg");
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, fc](const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                if (audioEngine.getMusicPlayer().loadFile(file))
                {
                    trackNameLabel.setText(file.getFileName(), juce::dontSendNotification);
                    audioEngine.getMusicPlayer().play();
                }
            }
        });
    }
    else if (button == &playMusicBtn)
    {
        audioEngine.getMusicPlayer().play();
    }
    else if (button == &pauseMusicBtn)
    {
        audioEngine.getMusicPlayer().pause();
    }
    else if (button == &stopMusicBtn)
    {
        audioEngine.getMusicPlayer().stop();
    }
    // SFX
    else if (button == &sfxLaughBtn) audioEngine.getSfxPlayer().trigger(SfxPlayer::BuiltinSound::Laugh);
    else if (button == &sfxApplauseBtn) audioEngine.getSfxPlayer().trigger(SfxPlayer::BuiltinSound::Applause);
    else if (button == &sfxCrowdBtn) audioEngine.getSfxPlayer().trigger(SfxPlayer::BuiltinSound::Crowd);
    else if (button == &sfxHornBtn) audioEngine.getSfxPlayer().trigger(SfxPlayer::BuiltinSound::Airhorn);
    // Recording
    else if (button == &recordBtn)
    {
        auto& rec = audioEngine.getAudioRecorder();
        if (rec.isRecording())
        {
            rec.stopRecording();
            recordBtn.setButtonText("● RECORD");
            recordBtn.setColour(juce::TextButton::buttonColourId, HNStudioTheme::neonRed.withAlpha(0.25f));
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Đã lưu bản thu âm",
                "File đã được lưu tại:\n" + rec.getCurrentFile().getFullPathName()
            );
        }
        else
        {
            auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
            auto recDir = docs.getChildFile("HNStudio_Recordings");
            rec.startRecording(recDir, 48000.0, 2);
            recordBtn.setButtonText("■ DỪNG GHI");
            recordBtn.setColour(juce::TextButton::buttonColourId, HNStudioTheme::neonRed);
        }
    }
    else if (button == &openRecordFolderBtn)
    {
        auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        auto recDir = docs.getChildFile("HNStudio_Recordings");
        if (!recDir.exists()) recDir.createDirectory();
        recDir.startAsProcess();
    }
}

void MainComponent::openPopup(const juce::String& title, juce::Component* comp, int w, int h)
{
    currentPopup = std::make_unique<NeonDialogWindow>(title + " - HNStudio Musik AI", comp, w, h);
}

void MainComponent::updateButtonGlows()
{
    auto& dsp = audioEngine.getDspChain();

    auto updateToggle = [](juce::TextButton& btn, bool active, const juce::String& prefix)
    {
        btn.setButtonText(active ? (prefix + ": ON") : (prefix + ": OFF"));
        btn.setColour(juce::TextButton::textColourOffId, active ? HNStudioTheme::neonCyan : HNStudioTheme::textDim);
        btn.setColour(juce::TextButton::buttonColourId, active ? HNStudioTheme::neonCyan.withAlpha(0.2f) : HNStudioTheme::bgCard);
    };

    updateToggle(gateToggle, dsp.getNoiseGate().isEnabled(), "GATE");
    updateToggle(compToggle, dsp.getCompressor().isEnabled(), "COMP");
    updateToggle(eqToggle, dsp.getEQ().isEnabled(), "EQ");
    updateToggle(deEsserToggle, dsp.getDeEsser().isEnabled(), "DE-ESS");
    updateToggle(reverbToggle, dsp.getReverb().isEnabled(), "REVERB");
    updateToggle(limiterToggle, dsp.getLimiter().isEnabled(), "LIMIT");
}

void MainComponent::timerCallback()
{
    // Auto Key status update
    auto& detector = audioEngine.getAutoKeyDetector();
    if (detector.isAutoKeyEnabled())
    {
        autoKeyButton.setButtonText("KEY: " + detector.getCurrentKeyName());
        autoKeyButton.setColour(juce::TextButton::textColourOffId, HNStudioTheme::neonGreen);
    }
    else
    {
        autoKeyButton.setButtonText("KEY: " + detector.getCurrentKeyName());
        autoKeyButton.setColour(juce::TextButton::textColourOffId, HNStudioTheme::neonMagenta);
    }

    // Music Player timeline
    auto& player = audioEngine.getMusicPlayer();
    if (player.isPlaying())
    {
        double pos = player.getCurrentPosition();
        double dur = player.getDuration();
        if (dur > 0.0)
        {
            musicTimelineSlider.setValue((pos / dur) * 100.0, juce::dontSendNotification);
            int curM = (int)pos / 60, curS = (int)pos % 60;
            int totM = (int)dur / 60, totS = (int)dur % 60;
            musicTimeLabel.setText(juce::String::formatted("%02d:%02d / %02d:%02d", curM, curS, totM, totS), juce::dontSendNotification);
        }
    }

    // Recording status
    auto& rec = audioEngine.getAudioRecorder();
    if (rec.isRecording())
    {
        recordTimeLabel.setText("● RECORDING: " + rec.getFormattedTime(), juce::dontSendNotification);
    }

    // Clipping LED check
    if (audioEngine.getAndResetClip())
    {
        clipFlashCounter = 10; // flash for 10 frames
    }
    else if (clipFlashCounter > 0)
    {
        --clipFlashCounter;
    }

    repaint();
}
