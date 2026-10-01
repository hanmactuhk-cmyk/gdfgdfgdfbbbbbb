#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class LimiterProcessor
{
public:
    LimiterProcessor() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        limiter.prepare(spec);
        limiter.reset();

        updateLimiter();
    }

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return enabled; }

    void setThreshold(float db)
    {
        thresholdDb = juce::jlimit(-24.0f, 0.0f, db);
        updateLimiter();
    }
    float getThreshold() const noexcept { return thresholdDb; }

    void setRelease(float ms)
    {
        releaseMs = juce::jlimit(2.0f, 1000.0f, ms);
        updateLimiter();
    }
    float getRelease() const noexcept { return releaseMs; }

    void setOutputCeiling(float db)
    {
        outputCeilingDb = juce::jlimit(-12.0f, 0.0f, db);
        outputLinear = juce::Decibels::decibelsToGain(outputCeilingDb);
    }
    float getOutputCeiling() const noexcept { return outputCeilingDb; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!enabled)
            return;

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        limiter.process(context);

        // Apply output ceiling and hard brickwall guard to guarantee zero digital overs
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float s = channelData[i] * outputLinear;
                if (s > outputLinear) s = outputLinear;
                else if (s < -outputLinear) s = -outputLinear;
                channelData[i] = s;
            }
        }
    }

private:
    void updateLimiter()
    {
        limiter.setThreshold(thresholdDb);
        limiter.setRelease(releaseMs);
    }

    double currentSampleRate = 44100.0;
    bool enabled = true;

    float thresholdDb = -0.5f;
    float releaseMs = 50.0f;
    float outputCeilingDb = -0.1f;
    float outputLinear = 0.9885f;

    juce::dsp::Limiter<float> limiter;
};
