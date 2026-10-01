#include "Popups.h"
#include <juce_gui_extra/juce_gui_extra.h>

// =============================================================================
// 1. Noise Gate Component
// =============================================================================
NoiseGateComponent::NoiseGateComponent(AudioEngine& engine) : audioEngine(engine)
{
    auto& gate = audioEngine.getDspChain().getNoiseGate();

    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(gate.isEnabled(), juce::dontSendNotification);
    enableToggle.addListener(this);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double val, const juce::String& suffix)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        s.setRange(min, max, 0.1);
        s.setValue(val, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        s.addListener(this);

        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);
    };

    setupSlider(thresholdSlider, thresholdLabel, "Threshold", -80.0, 0.0, gate.getThreshold(), " dB");
    setupSlider(attackSlider, attackLabel, "Attack", 0.1, 50.0, gate.getAttack(), " ms");
    setupSlider(releaseSlider, releaseLabel, "Release", 10.0, 1000.0, gate.getRelease(), " ms");
    setupSlider(rangeSlider, rangeLabel, "Range", -80.0, 0.0, gate.getRange(), " dB");
}

void NoiseGateComponent::resized()
{
    enableToggle.setBounds(20, 15, 200, 30);
    int startY = 60;
    int dialW = 100, dialH = 100;
    int spacing = 110;

    thresholdSlider.setBounds(20, startY, dialW, dialH);
    thresholdLabel.setBounds(20, startY + dialH + 5, dialW, 20);

    attackSlider.setBounds(20 + spacing, startY, dialW, dialH);
    attackLabel.setBounds(20 + spacing, startY + dialH + 5, dialW, 20);

    releaseSlider.setBounds(20 + spacing * 2, startY, dialW, dialH);
    releaseLabel.setBounds(20 + spacing * 2, startY + dialH + 5, dialW, 20);

    rangeSlider.setBounds(20 + spacing * 3, startY, dialW, dialH);
    rangeLabel.setBounds(20 + spacing * 3, startY + dialH + 5, dialW, 20);
}

void NoiseGateComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);
}

void NoiseGateComponent::sliderValueChanged(juce::Slider* slider)
{
    auto& gate = audioEngine.getDspChain().getNoiseGate();
    if (slider == &thresholdSlider) gate.setThreshold((float)thresholdSlider.getValue());
    else if (slider == &attackSlider) gate.setAttack((float)attackSlider.getValue());
    else if (slider == &releaseSlider) gate.setRelease((float)releaseSlider.getValue());
    else if (slider == &rangeSlider) gate.setRange((float)rangeSlider.getValue());
}

void NoiseGateComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableToggle)
        audioEngine.getDspChain().getNoiseGate().setEnabled(enableToggle.getToggleState());
}

// =============================================================================
// 2. Compressor Component
// =============================================================================
CompressorComponent::CompressorComponent(AudioEngine& engine) : audioEngine(engine)
{
    auto& comp = audioEngine.getDspChain().getCompressor();

    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(comp.isEnabled(), juce::dontSendNotification);
    enableToggle.addListener(this);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double val, const juce::String& suffix)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        s.setRange(min, max, 0.1);
        s.setValue(val, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        s.addListener(this);

        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);
    };

    setupSlider(threshSlider, threshLabel, "Threshold", -60.0, 0.0, comp.getThreshold(), " dB");
    setupSlider(ratioSlider, ratioLabel, "Ratio", 1.0, 20.0, comp.getRatio(), ":1");
    setupSlider(attackSlider, attackLabel, "Attack", 0.1, 100.0, comp.getAttack(), " ms");
    setupSlider(releaseSlider, releaseLabel, "Release", 10.0, 1000.0, comp.getRelease(), " ms");
    setupSlider(makeupSlider, makeupLabel, "Makeup", 0.0, 24.0, comp.getMakeupGain(), " dB");

    startTimerHz(30);
}

void CompressorComponent::resized()
{
    enableToggle.setBounds(20, 15, 200, 30);
    int startY = 60;
    int dialW = 90, dialH = 95;
    int spacing = 95;

    threshSlider.setBounds(20, startY, dialW, dialH);
    threshLabel.setBounds(20, startY + dialH, dialW, 20);

    ratioSlider.setBounds(20 + spacing, startY, dialW, dialH);
    ratioLabel.setBounds(20 + spacing, startY + dialH, dialW, 20);

    attackSlider.setBounds(20 + spacing * 2, startY, dialW, dialH);
    attackLabel.setBounds(20 + spacing * 2, startY + dialH, dialW, 20);

    releaseSlider.setBounds(20 + spacing * 3, startY, dialW, dialH);
    releaseLabel.setBounds(20 + spacing * 3, startY + dialH, dialW, 20);

    makeupSlider.setBounds(20 + spacing * 4, startY, dialW, dialH);
    makeupLabel.setBounds(20 + spacing * 4, startY + dialH, dialW, 20);
}

void CompressorComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);

    // Gain Reduction Meter
    float grDb = audioEngine.getDspChain().getCompressor().getCurrentGainReductionDb();
    int meterX = 20, meterY = 195, meterW = getWidth() - 40, meterH = 16;
    g.setColour(HNStudioTheme::bgCardHighlight);
    g.fillRoundedRectangle((float)meterX, (float)meterY, (float)meterW, (float)meterH, 3.0f);

    float normGr = juce::jlimit(0.0f, 1.0f, grDb / 20.0f);
    g.setColour(HNStudioTheme::neonMagenta);
    g.fillRoundedRectangle((float)meterX, (float)meterY, (float)meterW * normGr, (float)meterH, 3.0f);

    g.setColour(HNStudioTheme::textBright);
    g.setFont(12.0f);
    g.drawText("Gain Reduction: -" + juce::String(grDb, 1) + " dB", meterX, meterY - 18, 200, 18, juce::Justification::left);
}

void CompressorComponent::sliderValueChanged(juce::Slider* slider)
{
    auto& comp = audioEngine.getDspChain().getCompressor();
    if (slider == &threshSlider) comp.setThreshold((float)threshSlider.getValue());
    else if (slider == &ratioSlider) comp.setRatio((float)ratioSlider.getValue());
    else if (slider == &attackSlider) comp.setAttack((float)attackSlider.getValue());
    else if (slider == &releaseSlider) comp.setRelease((float)releaseSlider.getValue());
    else if (slider == &makeupSlider) comp.setMakeupGain((float)makeupSlider.getValue());
}

void CompressorComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableToggle)
        audioEngine.getDspChain().getCompressor().setEnabled(enableToggle.getToggleState());
}

// =============================================================================
// 3. Parametric EQ Component
// =============================================================================
EQComponent::EQComponent(AudioEngine& engine) : audioEngine(engine)
{
    auto& eq = audioEngine.getDspChain().getEQ();

    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(eq.isEnabled(), juce::dontSendNotification);
    enableToggle.addListener(this);

    static const char* labels[NumBands] = {
        "20Hz", "50Hz", "80Hz", "100Hz", "200Hz", "500Hz", "1kHz",
        "2kHz", "5kHz", "8kHz", "10kHz", "15kHz", "20kHz"
    };

    for (int i = 0; i < NumBands; ++i)
    {
        auto& s = gainSliders[i];
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 18);
        s.setRange(-18.0, 18.0, 0.5);
        s.setValue(eq.getBand(i).gainDb, juce::dontSendNotification);
        s.setTextValueSuffix(" dB");
        s.addListener(this);

        auto& l = freqLabels[i];
        addAndMakeVisible(l);
        l.setText(labels[i], juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(11.0f);
        l.setColour(juce::Label::textColourId, HNStudioTheme::neonCyan);
    }
}

void EQComponent::resized()
{
    enableToggle.setBounds(20, 10, 200, 26);
    int startY = 50;
    int availableW = getWidth() - 40;
    int bandW = availableW / NumBands;

    for (int i = 0; i < NumBands; ++i)
    {
        int x = 20 + i * bandW;
        freqLabels[i].setBounds(x, startY, bandW, 18);
        gainSliders[i].setBounds(x, startY + 22, bandW, 160);
    }
}

void EQComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);

    // Draw reference 0 dB line
    int y0 = 50 + 22 + 80;
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawDashedLine(juce::Line<float>(20.0f, (float)y0, (float)getWidth() - 20.0f, (float)y0), nullptr, 0, 1.0f);
}

void EQComponent::sliderValueChanged(juce::Slider* slider)
{
    auto& eq = audioEngine.getDspChain().getEQ();
    for (int i = 0; i < NumBands; ++i)
    {
        if (slider == &gainSliders[i])
        {
            auto b = eq.getBand(i);
            eq.setBand(i, b.frequency, (float)gainSliders[i].getValue(), b.q, b.enabled);
            break;
        }
    }
}

void EQComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableToggle)
        audioEngine.getDspChain().getEQ().setEnabled(enableToggle.getToggleState());
}

// =============================================================================
// 4. De-Esser Component
// =============================================================================
DeEsserComponent::DeEsserComponent(AudioEngine& engine) : audioEngine(engine)
{
    auto& de = audioEngine.getDspChain().getDeEsser();

    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(de.isEnabled(), juce::dontSendNotification);
    enableToggle.addListener(this);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double val, const juce::String& suffix)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 20);
        s.setRange(min, max, 1.0);
        s.setValue(val, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        s.addListener(this);

        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);
    };

    setupSlider(freqSlider, freqLabel, "Frequency", 3000.0, 11000.0, de.getFrequency(), " Hz");
    setupSlider(threshSlider, threshLabel, "Threshold", -60.0, 0.0, de.getThreshold(), " dB");
    setupSlider(amountSlider, amountLabel, "Amount", 0.0, 100.0, de.getAmount() * 100.0, " %");
}

void DeEsserComponent::resized()
{
    enableToggle.setBounds(20, 15, 200, 30);
    int startY = 60;
    int dialW = 100, dialH = 100;
    int spacing = 120;

    freqSlider.setBounds(30, startY, dialW, dialH);
    freqLabel.setBounds(30, startY + dialH + 5, dialW, 20);

    threshSlider.setBounds(30 + spacing, startY, dialW, dialH);
    threshLabel.setBounds(30 + spacing, startY + dialH + 5, dialW, 20);

    amountSlider.setBounds(30 + spacing * 2, startY, dialW, dialH);
    amountLabel.setBounds(30 + spacing * 2, startY + dialH + 5, dialW, 20);
}

void DeEsserComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);
}

void DeEsserComponent::sliderValueChanged(juce::Slider* slider)
{
    auto& de = audioEngine.getDspChain().getDeEsser();
    if (slider == &freqSlider) de.setFrequency((float)freqSlider.getValue());
    else if (slider == &threshSlider) de.setThreshold((float)threshSlider.getValue());
    else if (slider == &amountSlider) de.setAmount((float)amountSlider.getValue() / 100.0f);
}

void DeEsserComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableToggle)
        audioEngine.getDspChain().getDeEsser().setEnabled(enableToggle.getToggleState());
}

// =============================================================================
// 5. Reverb Component
// =============================================================================
ReverbComponent::ReverbComponent(AudioEngine& engine) : audioEngine(engine)
{
    auto& rev = audioEngine.getDspChain().getReverb();

    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(rev.isEnabled(), juce::dontSendNotification);
    enableToggle.addListener(this);

    addAndMakeVisible(presetCombo);
    presetCombo.addItem("Small Room", 1);
    presetCombo.addItem("Room", 2);
    presetCombo.addItem("Hall", 3);
    presetCombo.addItem("Large Hall", 4);
    presetCombo.addItem("Vocal", 5);
    presetCombo.addItem("Studio", 6);
    presetCombo.addItem("Plate", 7);
    presetCombo.setSelectedId(static_cast<int>(rev.getCurrentPreset()) + 1, juce::dontSendNotification);
    presetCombo.addListener(this);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double val, const juce::String& suffix)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 18);
        s.setRange(min, max, 0.01);
        s.setValue(val, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        s.addListener(this);

        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);
    };

    setupSlider(roomSizeSlider, roomSizeLabel, "Room Size", 0.0, 1.0, rev.getRoomSize(), "");
    setupSlider(dampSlider, dampLabel, "Damping", 0.0, 1.0, rev.getDamping(), "");
    setupSlider(widthSlider, widthLabel, "Width", 0.0, 1.0, rev.getWidth(), "");
    setupSlider(wetSlider, wetLabel, "Wet", 0.0, 1.0, rev.getWet(), "");
    setupSlider(drySlider, dryLabel, "Dry", 0.0, 1.0, rev.getDry(), "");
    setupSlider(preDelaySlider, preDelayLabel, "Pre Delay", 0.0, 200.0, rev.getPreDelay(), " ms");
}

void ReverbComponent::resized()
{
    enableToggle.setBounds(20, 15, 160, 28);
    presetCombo.setBounds(200, 15, 160, 28);

    int startY = 65;
    int dialW = 85, dialH = 90;
    int spacing = 95;

    roomSizeSlider.setBounds(20, startY, dialW, dialH);
    roomSizeLabel.setBounds(20, startY + dialH, dialW, 18);

    dampSlider.setBounds(20 + spacing, startY, dialW, dialH);
    dampLabel.setBounds(20 + spacing, startY + dialH, dialW, 18);

    widthSlider.setBounds(20 + spacing * 2, startY, dialW, dialH);
    widthLabel.setBounds(20 + spacing * 2, startY + dialH, dialW, 18);

    wetSlider.setBounds(20 + spacing * 3, startY, dialW, dialH);
    wetLabel.setBounds(20 + spacing * 3, startY + dialH, dialW, 18);

    drySlider.setBounds(20 + spacing * 4, startY, dialW, dialH);
    dryLabel.setBounds(20 + spacing * 4, startY + dialH, dialW, 18);

    preDelaySlider.setBounds(20 + spacing * 5, startY, dialW, dialH);
    preDelayLabel.setBounds(20 + spacing * 5, startY + dialH, dialW, 18);
}

void ReverbComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);
}

void ReverbComponent::sliderValueChanged(juce::Slider* slider)
{
    auto& rev = audioEngine.getDspChain().getReverb();
    if (slider == &roomSizeSlider) rev.setRoomSize((float)roomSizeSlider.getValue());
    else if (slider == &dampSlider) rev.setDamping((float)dampSlider.getValue());
    else if (slider == &widthSlider) rev.setWidth((float)widthSlider.getValue());
    else if (slider == &wetSlider) rev.setWet((float)wetSlider.getValue());
    else if (slider == &drySlider) rev.setDry((float)drySlider.getValue());
    else if (slider == &preDelaySlider) rev.setPreDelay((float)preDelaySlider.getValue());
}

void ReverbComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableToggle)
        audioEngine.getDspChain().getReverb().setEnabled(enableToggle.getToggleState());
}

void ReverbComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &presetCombo)
    {
        int p = presetCombo.getSelectedId() - 1;
        auto& rev = audioEngine.getDspChain().getReverb();
        rev.applyPreset(static_cast<ReverbProcessor::Preset>(p));

        roomSizeSlider.setValue(rev.getRoomSize(), juce::dontSendNotification);
        dampSlider.setValue(rev.getDamping(), juce::dontSendNotification);
        widthSlider.setValue(rev.getWidth(), juce::dontSendNotification);
        wetSlider.setValue(rev.getWet(), juce::dontSendNotification);
        drySlider.setValue(rev.getDry(), juce::dontSendNotification);
        preDelaySlider.setValue(rev.getPreDelay(), juce::dontSendNotification);
    }
}

// =============================================================================
// 6. Limiter Component
// =============================================================================
LimiterComponent::LimiterComponent(AudioEngine& engine) : audioEngine(engine)
{
    auto& lim = audioEngine.getDspChain().getLimiter();

    addAndMakeVisible(enableToggle);
    enableToggle.setToggleState(lim.isEnabled(), juce::dontSendNotification);
    enableToggle.addListener(this);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double val, const juce::String& suffix)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 20);
        s.setRange(min, max, 0.1);
        s.setValue(val, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        s.addListener(this);

        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);
    };

    setupSlider(threshSlider, threshLabel, "Threshold", -12.0, 0.0, lim.getThreshold(), " dB");
    setupSlider(releaseSlider, releaseLabel, "Release", 5.0, 500.0, lim.getRelease(), " ms");
    setupSlider(ceilingSlider, ceilingLabel, "Ceiling Output", -6.0, 0.0, lim.getOutputCeiling(), " dB");
}

void LimiterComponent::resized()
{
    enableToggle.setBounds(20, 15, 200, 30);
    int startY = 60;
    int dialW = 100, dialH = 100;
    int spacing = 120;

    threshSlider.setBounds(30, startY, dialW, dialH);
    threshLabel.setBounds(30, startY + dialH + 5, dialW, 20);

    releaseSlider.setBounds(30 + spacing, startY, dialW, dialH);
    releaseLabel.setBounds(30 + spacing, startY + dialH + 5, dialW, 20);

    ceilingSlider.setBounds(30 + spacing * 2, startY, dialW, dialH);
    ceilingLabel.setBounds(30 + spacing * 2, startY + dialH + 5, dialW, 20);
}

void LimiterComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);
}

void LimiterComponent::sliderValueChanged(juce::Slider* slider)
{
    auto& lim = audioEngine.getDspChain().getLimiter();
    if (slider == &threshSlider) lim.setThreshold((float)threshSlider.getValue());
    else if (slider == &releaseSlider) lim.setRelease((float)releaseSlider.getValue());
    else if (slider == &ceilingSlider) lim.setOutputCeiling((float)ceilingSlider.getValue());
}

void LimiterComponent::buttonClicked(juce::Button* button)
{
    if (button == &enableToggle)
        audioEngine.getDspChain().getLimiter().setEnabled(enableToggle.getToggleState());
}

// =============================================================================
// 7. Auto-Tune & Auto Key Component
// =============================================================================
AutoTuneComponent::AutoTuneComponent(AudioEngine& engine) : audioEngine(engine)
{
    addAndMakeVisible(autoKeyToggle);
    autoKeyToggle.setToggleState(audioEngine.getAutoKeyDetector().isAutoKeyEnabled(), juce::dontSendNotification);
    autoKeyToggle.addListener(this);

    addAndMakeVisible(keySelector);
    static const char* keys[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i)
        keySelector.addItem(keys[i], i + 1);
    keySelector.setSelectedId(audioEngine.getAutoKeyDetector().getCurrentKeyIndex() + 1, juce::dontSendNotification);
    keySelector.addListener(this);

    addAndMakeVisible(scaleSelector);
    scaleSelector.addItem("Major", 1);
    scaleSelector.addItem("Minor", 2);
    scaleSelector.setSelectedId(audioEngine.getAutoKeyDetector().isCurrentMajor() ? 1 : 2, juce::dontSendNotification);
    scaleSelector.addListener(this);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, double val, const juce::String& suffix)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 18);
        s.setRange(0.0, 100.0, 1.0);
        s.setValue(val, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        s.addListener(this);

        addAndMakeVisible(l);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, HNStudioTheme::textSecondary);
    };

    setupSlider(retuneSpeedSlider, retuneLabel, "Retune Speed", 20.0, "");
    setupSlider(humanizeSlider, humanizeLabel, "Humanize", 15.0, "");
    setupSlider(flexTuneSlider, flexLabel, "Flex-Tune", 10.0, "");
    setupSlider(mixSlider, mixLabel, "Mix", 100.0, " %");

    addAndMakeVisible(detectedKeyLabel);
    detectedKeyLabel.setText("Detected Key: -- | Confidence: 0%", juce::dontSendNotification);
    detectedKeyLabel.setColour(juce::Label::textColourId, HNStudioTheme::neonGreen);
    detectedKeyLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    addAndMakeVisible(syncStatusLabel);
    syncStatusLabel.setText("Trạng thái Auto-Tune VST: Sẵn sàng đồng bộ thông số Key.", juce::dontSendNotification);
    syncStatusLabel.setColour(juce::Label::textColourId, HNStudioTheme::textDim);

    startTimerHz(15);
}

void AutoTuneComponent::resized()
{
    autoKeyToggle.setBounds(20, 15, 160, 28);
    keySelector.setBounds(200, 15, 80, 28);
    scaleSelector.setBounds(290, 15, 100, 28);

    detectedKeyLabel.setBounds(20, 50, 400, 24);

    int startY = 85;
    int dialW = 85, dialH = 90;
    int spacing = 95;

    retuneSpeedSlider.setBounds(20, startY, dialW, dialH);
    retuneLabel.setBounds(20, startY + dialH, dialW, 18);

    humanizeSlider.setBounds(20 + spacing, startY, dialW, dialH);
    humanizeLabel.setBounds(20 + spacing, startY + dialH, dialW, 18);

    flexTuneSlider.setBounds(20 + spacing * 2, startY, dialW, dialH);
    flexLabel.setBounds(20 + spacing * 2, startY + dialH, dialW, 18);

    mixSlider.setBounds(20 + spacing * 3, startY, dialW, dialH);
    mixLabel.setBounds(20 + spacing * 3, startY + dialH, dialW, 18);

    syncStatusLabel.setBounds(20, 205, getWidth() - 40, 22);
}

void AutoTuneComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);
}

void AutoTuneComponent::sliderValueChanged(juce::Slider* /*slider*/)
{
}

void AutoTuneComponent::buttonClicked(juce::Button* button)
{
    if (button == &autoKeyToggle)
    {
        audioEngine.getAutoKeyDetector().setAutoKeyEnabled(autoKeyToggle.getToggleState());
    }
}

void AutoTuneComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &keySelector || comboBox == &scaleSelector)
    {
        int k = keySelector.getSelectedId() - 1;
        bool isMaj = (scaleSelector.getSelectedId() == 1);
        audioEngine.getAutoKeyDetector().setManualKey(k, isMaj);

        // Send to Auto-Tune VST
        auto results = audioEngine.getVstRack().sendKeyToPlugins(k, isMaj);
        if (!results.empty())
        {
            syncStatusLabel.setText(results[0].message, juce::dontSendNotification);
        }
    }
}

void AutoTuneComponent::timerCallback()
{
    auto& detector = audioEngine.getAutoKeyDetector();
    if (detector.isAutoKeyEnabled())
    {
        juce::String text = "Ước lượng: " + detector.getCurrentKeyName() +
                            " | Độ tin cậy: " + juce::String((int)detector.getConfidence()) + "%";
        detectedKeyLabel.setText(text, juce::dontSendNotification);

        // Auto update combobox display
        keySelector.setSelectedId(detector.getCurrentKeyIndex() + 1, juce::dontSendNotification);
        scaleSelector.setSelectedId(detector.isCurrentMajor() ? 1 : 2, juce::dontSendNotification);

        // Sync with VST
        auto results = audioEngine.getVstRack().sendKeyToPlugins(detector.getCurrentKeyIndex(), detector.isCurrentMajor());
        if (!results.empty())
        {
            syncStatusLabel.setText(results[0].message, juce::dontSendNotification);
        }
    }
}

// =============================================================================
// 8. Audio I/O Device Component
// =============================================================================
AudioIoComponent::AudioIoComponent(AudioEngine& engine)
{
    selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        engine.getDeviceManager(),
        1, 4, // min/max inputs
        1, 4, // min/max outputs
        true, // show audio type
        false, // midi input
        false, // midi output
        false  // stereo pairs
    );
    addAndMakeVisible(selector.get());
}

void AudioIoComponent::resized()
{
    if (selector != nullptr)
        selector->setBounds(getLocalBounds().reduced(10));
}

// =============================================================================
// 9. VST Manager Component
// =============================================================================
VstManagerComponent::PluginItemComponent::PluginItemComponent(VstRack& rack, int index)
    : vstRack(rack), slotIndex(index)
{
    const auto* slot = vstRack.getSlot(slotIndex);

    addAndMakeVisible(nameLabel);
    nameLabel.setText(juce::String(index + 1) + ". " + (slot ? slot->name : "VST"), juce::dontSendNotification);
    nameLabel.setColour(juce::Label::textColourId, HNStudioTheme::neonCyan);
    nameLabel.setFont(juce::Font(13.0f, juce::Font::bold));

    addAndMakeVisible(bypassButton);
    bypassButton.setToggleState(slot && slot->bypassed, juce::dontSendNotification);
    bypassButton.onClick = [this]() {
        vstRack.setBypass(slotIndex, bypassButton.getToggleState());
    };

    addAndMakeVisible(openButton);
    openButton.onClick = [this]() {
        vstRack.openEditor(slotIndex);
    };

    addAndMakeVisible(upButton);
    upButton.onClick = [this]() {
        vstRack.moveUp(slotIndex);
    };

    addAndMakeVisible(downButton);
    downButton.onClick = [this]() {
        vstRack.moveDown(slotIndex);
    };

    addAndMakeVisible(removeButton);
    removeButton.setColour(juce::TextButton::buttonColourId, HNStudioTheme::neonRed.withAlpha(0.2f));
    removeButton.setColour(juce::TextButton::textColourOffId, HNStudioTheme::neonRed);
    removeButton.onClick = [this]() {
        vstRack.removePlugin(slotIndex);
    };
}

void VstManagerComponent::PluginItemComponent::resized()
{
    int w = getWidth();
    nameLabel.setBounds(10, 5, w - 280, 30);
    bypassButton.setBounds(w - 270, 5, 65, 30);
    openButton.setBounds(w - 200, 5, 70, 30);
    upButton.setBounds(w - 125, 5, 35, 30);
    downButton.setBounds(w - 85, 5, 35, 30);
    removeButton.setBounds(w - 45, 5, 35, 30);
}

void VstManagerComponent::PluginItemComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgCard);
    g.setColour(HNStudioTheme::borderSubtle);
    g.drawRect(getLocalBounds(), 1);
}

int VstManagerComponent::ListModel::getNumRows()
{
    return ownerComp.audioEngine.getVstRack().getNumPlugins();
}

void VstManagerComponent::ListModel::paintListBoxItem(int /*rowNumber*/, juce::Graphics& /*g*/, int /*width*/, int /*height*/, bool /*rowIsSelected*/)
{
}

juce::Component* VstManagerComponent::ListModel::refreshComponentForRow(int rowNumber, bool /*isRowSelected*/, juce::Component* existingComponentToUpdate)
{
    delete existingComponentToUpdate;
    return new PluginItemComponent(ownerComp.audioEngine.getVstRack(), rowNumber);
}

VstManagerComponent::VstManagerComponent(AudioEngine& engine) : audioEngine(engine)
{
    audioEngine.getVstRack().addListener(this);

    addAndMakeVisible(addVstButton);
    addVstButton.setColour(juce::TextButton::buttonColourId, HNStudioTheme::neonCyan.withAlpha(0.2f));
    addVstButton.setColour(juce::TextButton::textColourOffId, HNStudioTheme::neonCyan);
    addVstButton.addListener(this);

    addAndMakeVisible(pluginListBox);
    pluginListBox.setModel(&listModel);
    pluginListBox.setRowHeight(44);
    pluginListBox.setColour(juce::ListBox::backgroundColourId, HNStudioTheme::bgPanel);
}

VstManagerComponent::~VstManagerComponent()
{
    audioEngine.getVstRack().removeListener(this);
}

void VstManagerComponent::resized()
{
    addVstButton.setBounds(20, 15, getWidth() - 40, 36);
    pluginListBox.setBounds(20, 60, getWidth() - 40, getHeight() - 75);
}

void VstManagerComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgPanel);
}

void VstManagerComponent::buttonClicked(juce::Button* button)
{
    if (button == &addVstButton)
    {
        auto fileChooser = std::make_shared<juce::FileChooser>(
            "Chọn file VST3 Plugin",
            juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory),
            "*.vst3"
        );

        auto folderFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(folderFlags, [this, fileChooser](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.exists())
            {
                juce::String err;
                if (!audioEngine.getVstRack().loadPlugin(result, err))
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Lỗi tải VST3",
                        "Không thể tải plugin: " + err
                    );
                }
            }
        });
    }
}

void VstManagerComponent::vstRackChanged()
{
    pluginListBox.updateContent();
    pluginListBox.repaint();
}

// =============================================================================
// 10. User Guide Component
// =============================================================================
UserGuideComponent::UserGuideComponent()
{
    addAndMakeVisible(guideText);
    guideText.setMultiLine(true);
    guideText.setReadOnly(true);
    guideText.setCaretVisible(false);
    guideText.setColour(juce::TextEditor::backgroundColourId, HNStudioTheme::bgPanel);
    guideText.setColour(juce::TextEditor::textColourId, HNStudioTheme::textBright);
    guideText.setFont(juce::Font(13.5f));

    juce::String guide =
        "✦ HƯỚNG DẪN SỬ DỤNG HNSTUDIO MUSIK AI ✦\n"
        "Tác giả: Hoài Nguyễn Studio | Zalo: 0965.043.000\n"
        "====================================================\n\n"
        "1. Cắm Microphone và Headphone / Audio Interface vào máy tính.\n"
        "2. Nhấn nút [AUDIO I/O] trên thanh điều khiển chính.\n"
        "3. Chọn Input Device (Mic USB hoặc Audio Interface).\n"
        "4. Chọn Output Device (Tai nghe hoặc Loa kiểm âm).\n"
        "5. Bật nút [LIVE ON] để kích hoạt luồng âm thanh thời gian thực.\n"
        "6. Chỉnh [MIC VOLUME] để tín hiệu đạt khoảng -6 dB đến -3 dB (tránh đèn đỏ CLIP).\n"
        "7. Nhấn [NOISE GATE] để lọc tạp âm môi trường và tiếng quạt.\n"
        "8. Nhấn [COMPRESSOR] để giữ giọng hát đồng đều, nội lực và dày dặn.\n"
        "9. Nhấn [EQ] để tăng sáng (high 8kHz - 15kHz) và ấm (100Hz - 200Hz).\n"
        "10. Nhấn [DE-ESSER] để giảm tiếng chói xì sibilance.\n"
        "11. Nhấn [REVERB] chọn Preset Vocal để có không gian hát mượt mà.\n"
        "12. Nhấn [+ ADD VST3] để nạp Auto-Tune VST3 (Antares, Waves Tune, FabFilter...).\n"
        "13. Chọn Key bài hát (C, D, G, Am...) hoặc bật [AUTO KEY].\n"
        "14. Bật [AUTO KEY DETECT] nếu muốn phần mềm tự động phân tích và dò tone bài hát.\n"
        "15. Trong phần MUSIC PLAYER: nhấn [MỞ NHẠC] để thêm file beat MP3/WAV/FLAC.\n"
        "16. Tùy chỉnh [MUSIC VOLUME] cân bằng với giọng hát.\n"
        "17. Nhấn [● RECORD] để ghi âm trọn vẹn buổi hát ra file WAV chất lượng cao 24-bit.\n\n"
        "--- NGUYÊN LÝ AUDIO ROUTING TÁCH BIỆT ---\n"
        "- MIC BUS: Mic In -> Noise Gate -> Comp -> EQ -> DeEsser -> Reverb -> VST3 -> Mic Master\n"
        "- MUSIC BUS: Music In / SFX -> Music Volume -> Music Master\n"
        "- MASTER BUS: Mic Master + Music Master -> Limiter chống vỡ -> Audio Out\n"
        "TUYỆT ĐỐI không cho beat chạy qua hiệu ứng của Mic!";

    guideText.setText(guide);
}

void UserGuideComponent::resized()
{
    guideText.setBounds(getLocalBounds().reduced(15));
}

void UserGuideComponent::paint(juce::Graphics& g)
{
    g.fillAll(HNStudioTheme::bgDark);
}
