#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class ReverbProcessor
{
public:
    enum class Preset
    {
        SmallRoom,
        Room,
        Hall,
        LargeHall,
        Vocal,
        Studio,
        Plate
    };

    ReverbProcessor()
    {
        applyPreset(Preset::Vocal);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        reverb.prepare(spec);
        reverb.reset();

        // 500 ms max pre-delay buffer
        int maxDelaySamples = (int)(currentSampleRate * 0.5);
        preDelayBuffer.setSize(2, maxDelaySamples);
        preDelayBuffer.clear();
        writePos = 0;

        updateParams();
    }

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return enabled; }

    void setRoomSize(float size) { roomSize = juce::jlimit(0.0f, 1.0f, size); updateParams(); }
    float getRoomSize() const noexcept { return roomSize; }

    void setDamping(float damp) { damping = juce::jlimit(0.0f, 1.0f, damp); updateParams(); }
    float getDamping() const noexcept { return damping; }

    void setWidth(float w) { width = juce::jlimit(0.0f, 1.0f, w); updateParams(); }
    float getWidth() const noexcept { return width; }

    void setWet(float wet) { wetLevel = juce::jlimit(0.0f, 1.0f, wet); updateParams(); }
    float getWet() const noexcept { return wetLevel; }

    void setDry(float dry) { dryLevel = juce::jlimit(0.0f, 1.0f, dry); updateParams(); }
    float getDry() const noexcept { return dryLevel; }

    void setPreDelay(float ms)
    {
        preDelayMs = juce::jlimit(0.0f, 250.0f, ms);
        delaySamples = (int)(currentSampleRate * 0.001 * preDelayMs);
    }
    float getPreDelay() const noexcept { return preDelayMs; }

    void applyPreset(Preset preset)
    {
        currentPreset = preset;
        switch (preset)
        {
            case Preset::SmallRoom:
                roomSize = 0.35f; damping = 0.65f; width = 0.70f; wetLevel = 0.25f; dryLevel = 1.0f; preDelayMs = 8.0f;
                break;
            case Preset::Room:
                roomSize = 0.50f; damping = 0.50f; width = 0.80f; wetLevel = 0.30f; dryLevel = 1.0f; preDelayMs = 15.0f;
                break;
            case Preset::Hall:
                roomSize = 0.75f; damping = 0.40f; width = 0.90f; wetLevel = 0.40f; dryLevel = 0.95f; preDelayMs = 30.0f;
                break;
            case Preset::LargeHall:
                roomSize = 0.92f; damping = 0.30f; width = 1.00f; wetLevel = 0.50f; dryLevel = 0.90f; preDelayMs = 45.0f;
                break;
            case Preset::Vocal:
                roomSize = 0.60f; damping = 0.45f; width = 0.85f; wetLevel = 0.35f; dryLevel = 1.0f; preDelayMs = 25.0f;
                break;
            case Preset::Studio:
                roomSize = 0.42f; damping = 0.70f; width = 0.75f; wetLevel = 0.22f; dryLevel = 1.0f; preDelayMs = 12.0f;
                break;
            case Preset::Plate:
                roomSize = 0.80f; damping = 0.20f; width = 1.00f; wetLevel = 0.38f; dryLevel = 1.0f; preDelayMs = 5.0f;
                break;
        }
        setPreDelay(preDelayMs);
        updateParams();
    }
    Preset getCurrentPreset() const noexcept { return currentPreset; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!enabled || wetLevel <= 0.001f)
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        const int bufLength = preDelayBuffer.getNumSamples();

        if (bufLength <= 0)
            return;

        // Apply pre-delay to input feed
        if (delaySamples > 0 && numChannels >= 1)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                int readPos = (writePos - delaySamples + bufLength) % bufLength;

                for (int ch = 0; ch < std::min(numChannels, 2); ++ch)
                {
                    float dryIn = buffer.getSample(ch, i);
                    preDelayBuffer.setSample(ch, writePos, dryIn);
                    float delayed = preDelayBuffer.getSample(ch, readPos);
                    // feed delayed into buffer for reverb
                    buffer.setSample(ch, i, dryIn * (1.0f - wetLevel) + delayed * wetLevel);
                }

                writePos = (writePos + 1) % bufLength;
            }
        }

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
    }

private:
    void updateParams()
    {
        juce::dsp::Reverb::Parameters p;
        p.roomSize = roomSize;
        p.damping = damping;
        p.wetLevel = wetLevel;
        p.dryLevel = dryLevel;
        p.width = width;
        p.freezeMode = 0.0f;
        reverb.setParameters(p);
    }

    double currentSampleRate = 44100.0;
    bool enabled = true;
    Preset currentPreset = Preset::Vocal;

    float roomSize = 0.60f;
    float damping = 0.45f;
    float width = 0.85f;
    float wetLevel = 0.35f;
    float dryLevel = 1.00f;
    float preDelayMs = 25.0f;

    int delaySamples = 0;
    int writePos = 0;
    juce::AudioBuffer<float> preDelayBuffer;
    juce::dsp::Reverb reverb;
};
