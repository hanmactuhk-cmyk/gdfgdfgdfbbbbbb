#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DspChain.h"
#include "VstRack.h"
#include "AutoKeyDetector.h"
#include "MusicPlayer.h"
#include "SfxPlayer.h"
#include "AudioRecorder.h"
#include <atomic>
#include <array>

class AudioEngine : public juce::AudioIODeviceCallback,
                    public juce::ChangeListener
{
public:
    AudioEngine();
    ~AudioEngine() override;

    bool initAudioDevice();
    void closeAudioDevice();

    // AudioIODeviceCallback
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceError(const juce::String& errorMessage) override;

    // ChangeListener (device changes)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Components
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    DspChain& getDspChain() { return dspChain; }
    VstRack& getVstRack() { return vstRack; }
    AutoKeyDetector& getAutoKeyDetector() { return autoKeyDetector; }
    MusicPlayer& getMusicPlayer() { return musicPlayer; }
    SfxPlayer& getSfxPlayer() { return sfxPlayer; }
    AudioRecorder& getAudioRecorder() { return audioRecorder; }

    // Volumes & Controls
    void setLiveEnabled(bool enabled) { isLiveOn.store(enabled); }
    bool isLiveEnabled() const { return isLiveOn.load(); }

    void setMicVolume(float vol) { micVolume.store(juce::jlimit(0.0f, 2.0f, vol)); }
    float getMicVolume() const { return micVolume.load(); }

    void setMusicVolume(float vol)
    {
        musicVolume.store(juce::jlimit(0.0f, 2.0f, vol));
        musicPlayer.setVolume(vol);
    }
    float getMusicVolume() const { return musicVolume.load(); }

    void setMasterVolume(float vol) { masterVolume.store(juce::jlimit(0.0f, 2.0f, vol)); }
    float getMasterVolume() const { return masterVolume.load(); }

    // Metering
    float getMicLevelPeak() const { return micLevelPeak.load(); }
    float getMicLevelRms() const { return micLevelRms.load(); }
    float getOutputLevelPeak() const { return outputLevelPeak.load(); }
    float getOutputLevelRms() const { return outputLevelRms.load(); }
    bool getAndResetClip() { return isClipping.exchange(false); }

    // Realtime Waveform extraction
    static constexpr int WaveformBufferSize = 512;
    void getMicWaveform(std::array<float, WaveformBufferSize>& dest);
    void getOutputWaveform(std::array<float, WaveformBufferSize>& dest);

    // Error & device disconnect notifications
    juce::String getLastError() const { return lastDeviceError; }
    bool hasDeviceError() const { return lastDeviceError.isNotEmpty(); }
    void clearDeviceError() { lastDeviceError = ""; }

private:
    juce::AudioDeviceManager deviceManager;
    DspChain dspChain;
    VstRack vstRack;
    AutoKeyDetector autoKeyDetector;
    MusicPlayer musicPlayer;
    SfxPlayer sfxPlayer;
    AudioRecorder audioRecorder;

    std::atomic<bool> isLiveOn { true };
    std::atomic<float> micVolume { 1.0f };
    std::atomic<float> musicVolume { 0.85f };
    std::atomic<float> masterVolume { 1.0f };

    std::atomic<float> micLevelPeak { 0.0f };
    std::atomic<float> micLevelRms { 0.0f };
    std::atomic<float> outputLevelPeak { 0.0f };
    std::atomic<float> outputLevelRms { 0.0f };
    std::atomic<bool> isClipping { false };

    // Separate processing audio buffers
    juce::AudioBuffer<float> micBusBuffer;
    juce::AudioBuffer<float> musicBusBuffer;
    juce::AudioBuffer<float> masterBusBuffer;
    juce::MidiBuffer midiBuffer;

    // Lock-free waveform FIFO
    std::array<float, WaveformBufferSize> micWaveformFifo{};
    std::array<float, WaveformBufferSize> outputWaveformFifo{};
    std::atomic<int> micFifoWritePos { 0 };
    std::atomic<int> outFifoWritePos { 0 };

    juce::String lastDeviceError;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
