#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

class AutoKeyDetector
{
public:
    AutoKeyDetector();
    ~AutoKeyDetector() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    void setAutoKeyEnabled(bool enabled) noexcept { autoKeyEnabled = enabled; }
    bool isAutoKeyEnabled() const noexcept { return autoKeyEnabled; }

    void setManualKey(int keyIndex, bool isMajor);
    
    int getCurrentKeyIndex() const noexcept { return currentKeyIndex; }
    bool isCurrentMajor() const noexcept { return currentIsMajor; }
    float getConfidence() const noexcept { return confidence; }
    juce::String getCurrentKeyName() const;

    static const char* getKeyName(int index);
    static int getNumKeys() noexcept { return 12; }

private:
    void analyzeBuffer();
    void estimateKeyFromChroma();

    double currentSampleRate = 44100.0;
    bool autoKeyEnabled = false;

    int currentKeyIndex = 0; // 0 = C, 1 = C#, ... 9 = A, etc.
    bool currentIsMajor = true;
    float confidence = 0.0f;

    // FFT & Analysis buffer
    static constexpr int fftOrder = 11; // 2048 samples
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;
    std::array<float, fftSize> fifo;

    // 12-element Chroma profile
    std::array<float, 12> chroma;
    int analysisCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoKeyDetector)
};
