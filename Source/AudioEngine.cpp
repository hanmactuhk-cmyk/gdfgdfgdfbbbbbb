#include "AudioEngine.h"
#include <cmath>

AudioEngine::AudioEngine()
{
    deviceManager.addChangeListener(this);
}

AudioEngine::~AudioEngine()
{
    closeAudioDevice();
    deviceManager.removeChangeListener(this);
}

bool AudioEngine::initAudioDevice()
{
    // Initialize with 2 inputs and 2 outputs, preferred 48kHz or 44.1kHz, 512 buffer
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    
    juce::String err = deviceManager.initialise(2, 2, nullptr, true, {}, &setup);
    if (err.isNotEmpty())
    {
        lastDeviceError = "Khởi tạo thiết bị âm thanh thất bại: " + err;
        return false;
    }

    deviceManager.addAudioCallback(this);
    return true;
}

void AudioEngine::closeAudioDevice()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr) return;

    currentSampleRate = device->getCurrentSampleRate();
    currentBlockSize = device->getCurrentBufferSizeSamples();

    micBusBuffer.setSize(2, currentBlockSize);
    musicBusBuffer.setSize(2, currentBlockSize);
    masterBusBuffer.setSize(2, currentBlockSize);

    dspChain.prepare(currentSampleRate, currentBlockSize);
    vstRack.prepare(currentSampleRate, currentBlockSize);
    autoKeyDetector.prepare(currentSampleRate, currentBlockSize);
    musicPlayer.prepare(currentSampleRate, currentBlockSize);
    sfxPlayer.prepare(currentSampleRate, currentBlockSize);

    lastDeviceError = "";
}

void AudioEngine::audioDeviceStopped()
{
    vstRack.release();
    musicPlayer.release();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                  int numInputChannels,
                                                  float* const* outputChannelData,
                                                  int numOutputChannels,
                                                  int numSamples,
                                                  const juce::AudioIODeviceCallbackContext& /*context*/)
{
    if (numSamples <= 0 || outputChannelData == nullptr || numOutputChannels == 0)
        return;

    // Check buffer allocation size
    if (micBusBuffer.getNumSamples() < numSamples)
    {
        micBusBuffer.setSize(2, numSamples, false, false, true);
        musicBusBuffer.setSize(2, numSamples, false, false, true);
        masterBusBuffer.setSize(2, numSamples, false, false, true);
    }

    micBusBuffer.clear();
    musicBusBuffer.clear();
    masterBusBuffer.clear();

    if (!isLiveOn.load())
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            if (outputChannelData[ch] != nullptr)
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        }
        micLevelPeak.store(0.0f);
        micLevelRms.store(0.0f);
        outputLevelPeak.store(0.0f);
        outputLevelRms.store(0.0f);
        return;
    }

    // =========================================================================
    // 1. MIC BUS (STRICT ISOLATION)
    // MIC INPUT -> Noise Gate -> Compressor -> EQ -> De-Esser -> Reverb -> VST3 RACK -> MIC MASTER
    // =========================================================================
    if (inputChannelData != nullptr && numInputChannels > 0)
    {
        // Read input channel(s)
        if (numInputChannels >= 2)
        {
            micBusBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);
            micBusBuffer.copyFrom(1, 0, inputChannelData[1], numSamples);
        }
        else
        {
            // Mono mic mapped to stereo
            micBusBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);
            micBusBuffer.copyFrom(1, 0, inputChannelData[0], numSamples);
        }

        // Pass strictly through DSP Chain:
        // Noise Gate -> Compressor -> EQ -> De-Esser -> Reverb
        dspChain.process(micBusBuffer);

        // VST3 Insert Rack (Auto-Tune, etc.)
        midiBuffer.clear();
        vstRack.process(micBusBuffer, midiBuffer);

        // Apply Mic Channel Volume
        float currentMicVol = micVolume.load();
        micBusBuffer.applyGain(currentMicVol);

        // Calculate Mic Level (Peak & RMS)
        float peakL = micBusBuffer.getMagnitude(0, 0, numSamples);
        float peakR = micBusBuffer.getMagnitude(1, 0, numSamples);
        float rmsL  = micBusBuffer.getRMSLevel(0, 0, numSamples);
        float rmsR  = micBusBuffer.getRMSLevel(1, 0, numSamples);
        micLevelPeak.store(std::max(peakL, peakR));
        micLevelRms.store(std::max(rmsL, rmsR));

        // Push to Mic Waveform FIFO (first channel)
        const auto* micSamples = micBusBuffer.getReadPointer(0);
        int writePos = micFifoWritePos.load();
        for (int i = 0; i < numSamples; ++i)
        {
            micWaveformFifo[(writePos + i) % WaveformBufferSize] = micSamples[i];
        }
        micFifoWritePos.store((writePos + numSamples) % WaveformBufferSize);

        // Feed to AutoKey Detector
        autoKeyDetector.process(micBusBuffer);
    }
    else
    {
        micLevelPeak.store(0.0f);
        micLevelRms.store(0.0f);
    }

    // =========================================================================
    // 2. MUSIC BUS (STRICT ISOLATION)
    // MUSIC FILE / INPUT -> MUSIC VOLUME -> MUSIC MASTER
    // =========================================================================
    juce::AudioSourceChannelInfo musicInfo(&musicBusBuffer, 0, numSamples);
    musicPlayer.getNextAudioBlock(musicInfo);

    // Sum SFX into music bus
    sfxPlayer.getNextAudioBlock(musicBusBuffer, 0, numSamples);

    // =========================================================================
    // 3. FINAL MASTER
    // MIC MASTER + MUSIC MASTER -> FINAL MASTER LIMITER -> AUDIO OUTPUT
    // =========================================================================
    masterBusBuffer.addFrom(0, 0, micBusBuffer, 0, 0, numSamples);
    masterBusBuffer.addFrom(1, 0, micBusBuffer, 1, 0, numSamples);

    masterBusBuffer.addFrom(0, 0, musicBusBuffer, 0, 0, numSamples);
    masterBusBuffer.addFrom(1, 0, musicBusBuffer, 1, 0, numSamples);

    // Master Limiter (Anti-Clipping)
    dspChain.getLimiter().process(masterBusBuffer);

    // Apply Master Volume
    float currentMasterVol = masterVolume.load();
    masterBusBuffer.applyGain(currentMasterVol);

    // Check clipping
    float outPeakL = masterBusBuffer.getMagnitude(0, 0, numSamples);
    float outPeakR = masterBusBuffer.getMagnitude(1, 0, numSamples);
    float outRmsL  = masterBusBuffer.getRMSLevel(0, 0, numSamples);
    float outRmsR  = masterBusBuffer.getRMSLevel(1, 0, numSamples);
    float maxOutPeak = std::max(outPeakL, outPeakR);

    outputLevelPeak.store(maxOutPeak);
    outputLevelRms.store(std::max(outRmsL, outRmsR));

    if (maxOutPeak >= 0.999f)
    {
        isClipping.store(true);
    }

    // Feed to realtime audio recorder (non-blocking FIFO)
    audioRecorder.processAudioBlock(masterBusBuffer);

    // Push to Output Waveform FIFO
    const auto* outSamples = masterBusBuffer.getReadPointer(0);
    int outWrite = outFifoWritePos.load();
    for (int i = 0; i < numSamples; ++i)
    {
        outputWaveformFifo[(outWrite + i) % WaveformBufferSize] = outSamples[i];
    }
    outFifoWritePos.store((outWrite + numSamples) % WaveformBufferSize);

    // Copy to physical device output channels
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            int srcCh = std::min(ch, 1);
            juce::FloatVectorOperations::copy(outputChannelData[ch],
                                              masterBusBuffer.getReadPointer(srcCh),
                                              numSamples);
        }
    }
}

void AudioEngine::audioDeviceError(const juce::String& errorMessage)
{
    lastDeviceError = "Lỗi thiết bị âm thanh: " + errorMessage;
}

void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    // Handle device changes/disconnects gracefully without crashing
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        lastDeviceError = "Thiết bị âm thanh đã bị ngắt kết nối. Vui lòng mở Audio I/O để chọn lại.";
    }
    else
    {
        lastDeviceError = "";
    }
}

void AudioEngine::getMicWaveform(std::array<float, WaveformBufferSize>& dest)
{
    int currentWrite = micFifoWritePos.load();
    for (int i = 0; i < WaveformBufferSize; ++i)
    {
        dest[i] = micWaveformFifo[(currentWrite + i) % WaveformBufferSize];
    }
}

void AudioEngine::getOutputWaveform(std::array<float, WaveformBufferSize>& dest)
{
    int currentWrite = outFifoWritePos.load();
    for (int i = 0; i < WaveformBufferSize; ++i)
    {
        dest[i] = outputWaveformFifo[(currentWrite + i) % WaveformBufferSize];
    }
}
