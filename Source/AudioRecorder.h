#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

class AudioRecorder : public juce::Thread
{
public:
    AudioRecorder() : juce::Thread("HNStudioAudioRecorderThread")
    {
    }

    ~AudioRecorder() override
    {
        stopRecording();
    }

    void startRecording(const juce::File& destinationFolder, double sampleRate, int numChannels = 2)
    {
        stopRecording();

        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        channels = numChannels;

        if (!destinationFolder.exists())
            destinationFolder.createDirectory();

        auto now = juce::Time::getCurrentTime();
        juce::String fileName = "HNStudio_Record_" + now.formatted("%Y-%m-%d_%H-%M-%S") + ".wav";
        currentRecordFile = destinationFolder.getChildFile(fileName);

        fifo.setSize(numChannels, (int)(currentSampleRate * 5.0)); // 5 seconds buffer
        abstractFifo.setTotalSize(fifo.getNumSamples());
        abstractFifo.reset();

        std::unique_ptr<juce::FileOutputStream> fileStream(currentRecordFile.createOutputStream());
        if (fileStream != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            writer.reset(wavFormat.createWriterFor(fileStream.get(), currentSampleRate,
                                                   (unsigned int)channels, 24, {}, 0));
            if (writer != nullptr)
            {
                fileStream.release();
                isRecordingActive = true;
                isPausedState = false;
                recordedSamplesCount = 0;
                startThread();
            }
        }
    }

    void pauseRecording()
    {
        isPausedState = true;
    }

    void resumeRecording()
    {
        isPausedState = false;
    }

    void stopRecording()
    {
        if (isRecordingActive)
        {
            isRecordingActive = false;
            stopThread(3000);
            writer.reset();
        }
    }

    bool isRecording() const noexcept { return isRecordingActive && !isPausedState; }
    bool isPaused() const noexcept { return isRecordingActive && isPausedState; }

    double getRecordedSeconds() const noexcept
    {
        return (double)recordedSamplesCount.get() / (currentSampleRate > 0.0 ? currentSampleRate : 44100.0);
    }

    juce::String getFormattedTime() const
    {
        int totalSec = (int)getRecordedSeconds();
        int mins = totalSec / 60;
        int secs = totalSec % 60;
        return juce::String::formatted("%02d:%02d", mins, secs);
    }

    juce::File getCurrentFile() const { return currentRecordFile; }

    // Called on audio thread - strictly lock-free!
    void processAudioBlock(const juce::AudioBuffer<float>& buffer)
    {
        if (!isRecordingActive || isPausedState)
            return;

        int numSamples = buffer.getNumSamples();
        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite(numSamples, start1, size1, start2, size2);

        if (size1 > 0)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                int srcCh = std::min(ch, buffer.getNumChannels() - 1);
                fifo.copyFrom(ch, start1, buffer, srcCh, 0, size1);
            }
        }

        if (size2 > 0)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                int srcCh = std::min(ch, buffer.getNumChannels() - 1);
                fifo.copyFrom(ch, start2, buffer, srcCh, size1, size2);
            }
        }

        abstractFifo.finishedWrite(size1 + size2);
        recordedSamplesCount += numSamples;
        notify();
    }

private:
    void run() override
    {
        while (!threadShouldExit())
        {
            int numReady = abstractFifo.getNumReady();
            if (numReady > 256 || !isRecordingActive)
            {
                int start1, size1, start2, size2;
                abstractFifo.prepareToRead(numReady, start1, size1, start2, size2);

                if (writer != nullptr)
                {
                    if (size1 > 0)
                        writer->writeFromAudioSampleBuffer(fifo, start1, size1);
                    if (size2 > 0)
                        writer->writeFromAudioSampleBuffer(fifo, start2, size2);
                }

                abstractFifo.finishedRead(size1 + size2);
            }
            else
            {
                wait(20);
            }

            if (!isRecordingActive && abstractFifo.getNumReady() == 0)
                break;
        }

        // Flush remaining
        int remaining = abstractFifo.getNumReady();
        if (remaining > 0 && writer != nullptr)
        {
            int start1, size1, start2, size2;
            abstractFifo.prepareToRead(remaining, start1, size1, start2, size2);
            if (size1 > 0) writer->writeFromAudioSampleBuffer(fifo, start1, size1);
            if (size2 > 0) writer->writeFromAudioSampleBuffer(fifo, start2, size2);
            abstractFifo.finishedRead(size1 + size2);
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
