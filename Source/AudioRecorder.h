#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <cstdint>

class AudioRecorder : public juce::Thread
{
public:
    AudioRecorder()
        : juce::Thread("HNStudioAudioRecorderThread")
    {
    }

    ~AudioRecorder() override
    {
        stopRecording();
    }

    void startRecording(const juce::File& destinationFolder,
                        double sampleRate,
                        int numChannels = 2)
    {
        stopRecording();

        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        channels = juce::jmax(1, numChannels);

        if (!destinationFolder.exists())
            destinationFolder.createDirectory();

        auto time = juce::Time::getCurrentTime();

        juce::String fileName =
            "HNStudio_Record_" +
            time.formatted("%Y-%m-%d_%H-%M-%S") +
            ".wav";

        currentRecordFile = destinationFolder.getChildFile(fileName);

        std::unique_ptr<juce::FileOutputStream> fileStream(
            currentRecordFile.createOutputStream());

        if (fileStream == nullptr)
            return;

        juce::WavAudioFormat wavFormat;

        writer.reset(
            wavFormat.createWriterFor(
                fileStream.get(),
                currentSampleRate,
                static_cast<unsigned int>(channels),
                24,
                {},
                0));

        if (writer == nullptr)
            return;

        fileStream.release();

        isRecordingActive.store(true, std::memory_order_release);
        isPausedState.store(false, std::memory_order_release);
        recordedSamplesCount.store(0, std::memory_order_release);

        startThread();
    }

    void pauseRecording()
    {
        isPausedState.store(true, std::memory_order_release);
    }

    void resumeRecording()
    {
        isPausedState.store(false, std::memory_order_release);
    }

    void stopRecording()
    {
        isRecordingActive.store(false, std::memory_order_release);

        stopThread(3000);

        writer.reset();
    }

    bool isRecording() const noexcept
    {
        return isRecordingActive.load(std::memory_order_acquire);
    }

    bool isPaused() const noexcept
    {
        return isPausedState.load(std::memory_order_acquire);
    }

    double getRecordedSeconds() const noexcept
    {
        const auto samples =
            recordedSamplesCount.load(std::memory_order_acquire);

        const double rate =
            currentSampleRate > 0.0
                ? currentSampleRate
                : 44100.0;

        return static_cast<double>(samples) / rate;
    }

    juce::String getFormattedTime() const
    {
        const int totalSeconds =
            static_cast<int>(getRecordedSeconds());

        const int minutes = totalSeconds / 60;
        const int seconds = totalSeconds % 60;

        return juce::String::formatted(
            "%02d:%02d",
            minutes,
            seconds);
    }

    juce::File getCurrentFile() const
    {
        return currentRecordFile;
    }

    void processAudioBlock(
        const juce::AudioBuffer<float>& buffer)
    {
        if (!isRecordingActive.load(std::memory_order_acquire))
            return;

        if (isPausedState.load(std::memory_order_acquire))
            return;

        const int numSamples = buffer.getNumSamples();

        if (numSamples <= 0)
            return;

        if (buffer.getNumChannels() <= 0)
            return;

        if (writer != nullptr)
        {
            writer->writeFromAudioSampleBuffer(
                buffer,
                0,
                numSamples);

            recordedSamplesCount.fetch_add(
                static_cast<int64_t>(numSamples),
                std::memory_order_relaxed);
        }
    }

private:
    void run() override
    {
        while (!threadShouldExit())
        {
            if (!isRecordingActive.load(std::memory_order_acquire))
                break;

            wait(20);
        }
    }

    std::unique_ptr<juce::AudioFormatWriter> writer;

    juce::File currentRecordFile;

    double currentSampleRate = 44100.0;

    int channels = 2;

    std::atomic<bool> isRecordingActive { false };

    std::atomic<bool> isPausedState { false };

    std::atomic<int64_t> recordedSamplesCount { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};
