```cpp
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <cstdint>
#include <algorithm>

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

        auto now = juce::Time::getCurrentTime();

        juce::String fileName =
            "HNStudio_Record_"
            + now.formatted("%Y-%m-%d_%H-%M-%S")
            + ".wav";

        currentRecordFile = destinationFolder.getChildFile(fileName);

        const int bufferSize =
            juce::jmax(1024, (int)(currentSampleRate * 5.0));

        fifo.setSize(channels, bufferSize);
        abstractFifo.setTotalSize(bufferSize);
        abstractFifo.reset();

        std::unique_ptr<juce::FileOutputStream> fileStream(
            currentRecordFile.createOutputStream());

        if (fileStream != nullptr)
        {
            juce::WavAudioFormat wavFormat;

            writer.reset(
                wavFormat.createWriterFor(
                    fileStream.get(),
                    currentSampleRate,
                    (unsigned int)channels,
                    24,
                    {},
                    0));

            if (writer != nullptr)
            {
                // AudioFormatWriter takes ownership of the stream.
                fileStream.release();

                isRecordingActive.store(true, std::memory_order_release);
                isPausedState.store(false, std::memory_order_release);
                recordedSamplesCount.store(0, std::memory_order_release);

                startThread();
            }
        }
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
        if (isRecordingActive.load(std::memory_order_acquire))
        {
            isRecordingActive.store(false, std::memory_order_release);

            stopThread(3000);

            writer.reset();
        }
    }

    bool isRecording() const noexcept
    {
        return isRecordingActive.load(std::memory_order_acquire)
            && !isPausedState.load(std::memory_order_acquire);
    }

    bool isPaused() const noexcept
    {
        return isRecordingActive.load(std::memory_order_acquire)
            && isPausedState.load(std::memory_order_acquire);
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
        const int totalSec =
            static_cast<int>(getRecordedSeconds());

        const int mins = totalSec / 60;
        const int secs = totalSec % 60;

        return juce::String::formatted(
            "%02d:%02d",
            mins,
            secs);
    }

    juce::File getCurrentFile() const
    {
        return currentRecordFile;
    }

    // Called from the audio thread.
    // This function does not perform file I/O.
    void processAudioBlock(
        const juce::AudioBuffer<float>& buffer)
    {
        if (!isRecordingActive.load(std::memory_order_acquire)
            || isPausedState.load(std::memory_order_acquire))
        {
            return;
        }

        const int numSamples = buffer.getNumSamples();

        if (numSamples <= 0 || buffer.getNumChannels() <= 0)
            return;

        int start1 = 0;
        int size1 = 0;
        int start2 = 0;
        int size2 = 0;

        abstractFifo.prepareToWrite(
            numSamples,
            start1,
            size1,
            start2,
            size2);

        if (size1 > 0)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                const int srcCh =
                    juce::jmin(
                        ch,
                        buffer.getNumChannels() - 1);

                fifo.copyFrom(
                    ch,
                    start1,
                    buffer,
                    srcCh,
                    0,
                    size1);
            }
        }

        if (size2 > 0)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                const int srcCh =
                    juce::jmin(
                        ch,
                        buffer.getNumChannels() - 1);

                fifo.copyFrom(
                    ch,
                    start2,
                    buffer,
                    srcCh,
                    size1,
                    size2);
            }
        }

        const int written = size1 + size2;

        if (written > 0)
        {
            abstractFifo.finishedWrite(written);

            recordedSamplesCount.fetch_add(
                written,
                std::memory_order_relaxed);

            notify();
        }
    }

private:
    void run() override
    {
        while (!threadShouldExit())
        {
            const int numReady =
                abstractFifo.getNumReady();

            if (numReady > 256
                || !isRecordingActive.load(
                    std::memory_order_acquire))
            {
                int start1 = 0;
                int size1 = 0;
                int start2 = 0;
                int size2 = 0;

                abstractFifo.prepareToRead(
                    numReady,
                    start1,
                    size1,
                    start2,
                    size2);

                if (writer != nullptr)
                {
                    if (size1 > 0)
                    {
                        writer->writeFromAudioSampleBuffer(
                            fifo,
                            start1,
                            size1);
                    }

                    if (size2 > 0)
                    {
                        writer->writeFromAudioSampleBuffer(
                            fifo,
                            start2,
                            size2);
                    }
                }

                abstractFifo.finishedRead(
                    size1 + size2);
            }
            else
            {
                wait(20);
            }

            if (!isRecordingActive.load(
                    std::memory_order_acquire)
                && abstractFifo.getNumReady() == 0)
            {
                break;
            }
        }

        // Flush any remaining samples before the thread exits.
        const int remaining =
            abstractFifo.getNumReady();

        if (remaining > 0 && writer != nullptr)
        {
            int start1 = 0;
            int size1 = 0;
            int start2 = 0;
            int size2 = 0;

            abstractFifo.prepareToRead(
                remaining,
                start1,
                size1,
                start2,
                size2);

            if (size1 > 0)
            {
                writer->writeFromAudioSampleBuffer(
                    fifo,
                    start1,
                    size1);
            }

            if (size2 > 0)
            {
                writer->writeFromAudioSampleBuffer(
                    fifo,
                    start2,
                    size2);
            }

            abstractFifo.finishedRead(
                size1 + size2);
        }
    }

    std::unique_ptr<juce::AudioFormatWriter> writer;

    juce::File currentRecordFile;

    double currentSampleRate = 44100.0;
    int channels = 2;

    std::atomic<bool> isRecordingActive { false };
    std::atomic<bool> isPausedState { false };
    std::atomic<int64_t> recordedSamplesCount { 0 };

    juce::AbstractFifo abstractFifo { 1024 };
    juce::AudioBuffer<float> fifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};
```
