#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "AudioEngine.h"
#include "GuiTheme.h"

// Base Neon Dialog Window
class NeonDialogWindow : public juce::DocumentWindow
{
public:
    NeonDialogWindow(const juce::String& name, juce::Component* contentComponent, int width, int height)
        : juce::DocumentWindow(name, HNStudioTheme::bgDark, juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(contentComponent, true);
        centreWithSize(width, height);
        setVisible(true);
        toFront(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};

// 1. Noise Gate Popup Component
class NoiseGateComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener
{
public:
    NoiseGateComponent(AudioEngine& engine);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    AudioEngine& audioEngine;
    juce::ToggleButton enableToggle { "BẬT NOISE GATE" };
    juce::Slider thresholdSlider, attackSlider, releaseSlider, rangeSlider;
    juce::Label thresholdLabel, attackLabel, releaseLabel, rangeLabel;
};

// 2. Compressor Popup Component
class CompressorComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener, public juce::Timer
{
public:
    CompressorComponent(AudioEngine& engine);
    ~CompressorComponent() override { stopTimer(); }
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override { repaint(); }

private:
    AudioEngine& audioEngine;
    juce::ToggleButton enableToggle { "BẬT COMPRESSOR" };
    juce::Slider threshSlider, ratioSlider, attackSlider, releaseSlider, makeupSlider;
    juce::Label threshLabel, ratioLabel, attackLabel, releaseLabel, makeupLabel;
};

// 3. EQ Popup Component
class EQComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener
{
public:
    EQComponent(AudioEngine& engine);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    AudioEngine& audioEngine;
    juce::ToggleButton enableToggle { "BẬT PARAMETRIC EQ" };
    static constexpr int NumBands = ParametricEQ::NumBands;
    std::array<juce::Slider, NumBands> gainSliders;
    std::array<juce::Label, NumBands> freqLabels;
};

// 4. De-Esser Popup Component
class DeEsserComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener
{
public:
    DeEsserComponent(AudioEngine& engine);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    AudioEngine& audioEngine;
    juce::ToggleButton enableToggle { "BẬT DE-ESSER" };
    juce::Slider freqSlider, threshSlider, amountSlider;
    juce::Label freqLabel, threshLabel, amountLabel;
};

// 5. Reverb Popup Component
class ReverbComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener, public juce::ComboBox::Listener
{
public:
    ReverbComponent(AudioEngine& engine);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

private:
    AudioEngine& audioEngine;
    juce::ToggleButton enableToggle { "BẬT REVERB" };
    juce::ComboBox presetCombo;
    juce::Slider roomSizeSlider, dampSlider, widthSlider, wetSlider, drySlider, preDelaySlider;
    juce::Label roomSizeLabel, dampLabel, widthLabel, wetLabel, dryLabel, preDelayLabel;
};

// 6. Limiter Popup Component
class LimiterComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener
{
public:
    LimiterComponent(AudioEngine& engine);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    AudioEngine& audioEngine;
    juce::ToggleButton enableToggle { "BẬT LIMITER" };
    juce::Slider threshSlider, releaseSlider, ceilingSlider;
    juce::Label threshLabel, releaseLabel, ceilingLabel;
};

// 7. Auto-Tune & Auto Key Popup Component
class AutoTuneComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener, public juce::ComboBox::Listener, public juce::Timer
{
public:
    AutoTuneComponent(AudioEngine& engine);
    ~AutoTuneComponent() override { stopTimer(); }
    void resized() override;
    void paint(juce::Graphics& g) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void timerCallback() override;

private:
    AudioEngine& audioEngine;
    juce::ToggleButton autoKeyToggle { "AUTO KEY DETECT" };
    juce::ComboBox keySelector;
    juce::ComboBox scaleSelector;
    juce::Slider retuneSpeedSlider, humanizeSlider, flexTuneSlider, mixSlider;
    juce::Label retuneLabel, humanizeLabel, flexLabel, mixLabel;
    juce::Label detectedKeyLabel;
    juce::Label syncStatusLabel;
};

// 8. Audio I/O Device Popup Component
class AudioIoComponent : public juce::Component
{
public:
    AudioIoComponent(AudioEngine& engine);
    void resized() override;

private:
    std::unique_ptr<juce::AudioDeviceSelectorComponent> selector;
};

// 9. VST Manager Popup Component
class VstManagerComponent : public juce::Component, public juce::Button::Listener, public VstRack::Listener
{
public:
    VstManagerComponent(AudioEngine& engine);
    ~VstManagerComponent() override;
    void resized() override;
    void paint(juce::Graphics& g) override;
    void buttonClicked(juce::Button* button) override;
    void vstRackChanged() override;

private:
    void rebuildList();

    AudioEngine& audioEngine;
    juce::TextButton addVstButton { "+ THÊM VST3 PLUGIN (.vst3)" };
    juce::ListBox pluginListBox;

    struct PluginItemComponent : public juce::Component
    {
        PluginItemComponent(VstRack& rack, int index);
        void resized() override;
        void paint(juce::Graphics& g) override;

        VstRack& vstRack;
        int slotIndex;
        juce::Label nameLabel;
        juce::ToggleButton bypassButton { "Bypass" };
        juce::TextButton openButton { "Mở VST" };
        juce::TextButton upButton { "▲" };
        juce::TextButton downButton { "▼" };
        juce::TextButton removeButton { "✕" };
    };

    class ListModel : public juce::ListBoxModel
    {
    public:
        ListModel(VstManagerComponent& owner) : ownerComp(owner) {}
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    private:
        VstManagerComponent& ownerComp;
    };

    ListModel listModel { *this };
};

// 10. User Guide Popup Component
class UserGuideComponent : public juce::Component
{
public:
    UserGuideComponent();
    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    juce::TextEditor guideText;
};
