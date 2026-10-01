#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <mutex>

class SfxPlayer
{
public:
    enum class BuiltinSound
    {
        Laugh,
        Applause,
        Crowd,
        Airhorn
    };

    struct SfxItem
    {
        juce::String name;
        juce::AudioBuffer<float> buffer;
        int playHead = -1;
    };

    SfxPlayer()
    {
        formatManager.registerBasicFormats();
        initDefaultSynthesizedSounds();
    }

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    void setVolume(float v) { sfxVolume = juce::jlimit(0.0f, 2.0f, v); }
    float getVolume() const noexcept { return sfxVolume; }

    void trigger(BuiltinSound sound)
    {
        int index = static_cast<int>(sound);
        triggerIndex(index);
    }

    void triggerIndex(int index)
    {
        std::lock_guard<std::mutex> lock(sfxMutex);
        if (index >= 0 && index < (int)soundList.size())
        {
            soundList[index].playHead = 0;
        }
    }

    bool loadUserFile(const juce::File& file, const juce::String& customName)
    {
        auto* reader = formatManager.createReaderFor(file);
        if (reader != nullptr)
        {
            std::unique_ptr<juce::AudioFormatReader> readerPtr(reader);
            juce::AudioBuffer<float> newBuf(2, (int)reader->lengthInSamples);
            reader->read(&newBuf, 0, (int)reader->lengthInSamples, 0, true, true);

            std::lock_guard<std::mutex> lock(sfxMutex);
            SfxItem item;
            item.name = customName.isNotEmpty() ? customName : file.getFileNameWithoutExtension();
            item.buffer = std::move(newBuf);
            item.playHead = -1;
            soundList.push_back(std::move(item));
            return true;
        }
        return false;
    }

    void getNextAudioBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
    {
        std::lock_guard<std::mutex> lock(sfxMutex);

        const int numOutChannels = outputBuffer.getNumChannels();

        for (auto& sound : soundList)
        {
            if (sound.playHead >= 0)
            {
                const int soundLength = sound.buffer.getNumSamples();
                const int remainingInSound = soundLength - sound.playHead;
                const int samplesToPlay = std::min(numSamples, remainingInSound);

                if (samplesToPlay > 0)
                {
                    for (int ch = 0; ch < numOutChannels; ++ch)
                    {
                        int srcCh = std::min(ch, sound.buffer.getNumChannels() - 1);
                        outputBuffer.addFrom(ch, startSample, sound.buffer, srcCh, sound.playHead, samplesToPlay, sfxVolume);
                    }
                    sound.playHead += samplesToPlay;
                }

                if (sound.playHead >= soundLength)
                {
                    sound.playHead = -1; // finished
                }
            }
        }
    }

    int getNumSounds() const
    {
        return (int)soundList.size();
    }

    juce::String getSoundName(int index) const
    {
        if (index >= 0 && index < (int)soundList.size())
            return soundList[index].name;
        return {};
    }

private:
    void initDefaultSynthesizedSounds()
    {
        // 1. Airhorn synth sound (classic pitch drop fanfare)
        {
            SfxItem airhorn;
            airhorn.name = "Airhorn";
            int samples = (int)(44100 * 1.2);
            airhorn.buffer.setSize(2, samples);
            for (int i = 0; i < samples; ++i)
            {
                float t = (float)i / 44100.0f;
                float freq = (t < 0.3f) ? 466.16f : ((t < 0.6f) ? 466.16f : ((t < 0.9f) ? 587.33f : 466.16f));
                float wave = std::sin(2.0f * juce::MathConstants<float>::pi * freq * t);
                wave += 0.5f * std::sin(4.0f * juce::MathConstants<float>::pi * freq * t);
                float env = (t > 1.0f) ? (1.2f - t) / 0.2f : 1.0f;
                float sample = wave * env * 0.4f;
                airhorn.buffer.setSample(0, i, sample);
                airhorn.buffer.setSample(1, i, sample);
            }
            soundList.push_back(std::move(airhorn));
        }

        // 2. Applause synth sound (filtered noise cluster)
        {
            SfxItem applause;
            applause.name = "Applause";
            int samples = (int)(44100 * 2.5);
            applause.buffer.setSize(2, samples);
            juce::Random rnd;
            for (int i = 0; i < samples; ++i)
            {
                float t = (float)i / 44100.0f;
                float noise = (rnd.nextFloat() * 2.0f - 1.0f) * 0.25f;
                float claps = (std::sin(t * 18.0f) > 0.8f ? 1.5f : 0.6f);
                float env = (t < 0.2f) ? (t / 0.2f) : ((t > 1.8f) ? (2.5f - t) / 0.7f : 1.0f);
                float sample = noise * claps * env;
                applause.buffer.setSample(0, i, sample);
                applause.buffer.setSample(1, i, sample);
            }
            soundList.push_back(std::move(applause));
        }

        // 3. Laugh synth sound
        {
            SfxItem laugh;
            laugh.name = "Laugh";
            int samples = (int)(44100 * 2.0);
            laugh.buffer.setSize(2, samples);
            for (int i = 0; i < samples; ++i)
            {
                float t = (float)i / 44100.0f;
                float chuckle = std::pow(std::abs(std::sin(t * 8.0f * juce::MathConstants<float>::pi)), 3.0f);
                float wave = std::sin(2.0f * juce::MathConstants<float>::pi * 320.0f * t);
                float env = (t < 1.5f) ? 1.0f : (2.0f - t) / 0.5f;
                float sample = wave * chuckle * env * 0.35f;
                laugh.buffer.setSample(0, i, sample);
                laugh.buffer.setSample(1, i, sample);
            }
            soundList.push_back(std::move(laugh));
        }

        // 4. Crowd cheering
        {
            SfxItem crowd;
            crowd.name = "Crowd";
            int samples = (int)(44100 * 2.8);
            crowd.buffer.setSize(2, samples);
            juce::Random rnd;
            for (int i = 0; i < samples; ++i)
            {
                float t = (float)i / 44100.0f;
                float noise = (rnd.nextFloat() * 2.0f - 1.0f) * 0.3f;
                float swell = std::sin(t * 1.5f);
                float env = (t < 0.5f) ? (t / 0.5f) : ((t > 2.0f) ? (2.8f - t) / 0.8f : 1.0f);
                float sample = noise * swell * env * 0.35f;
                crowd.buffer.setSample(0, i, sample);
                crowd.buffer.setSample(1, i, sample);
            }
            soundList.push_back(std::move(crowd));
        }
    }

    double currentSampleRate = 44100.0;
    float sfxVolume = 0.8f;
    std::vector<SfxItem> soundList;
    std::mutex sfxMutex;
    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SfxPlayer)
};
