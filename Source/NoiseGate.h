#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class NoiseGate
{
public:
    NoiseGate() = default;

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        updateTimeConstants();
        envelope = 0.0f;
        gainReduction = 1.0f;
    }

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return enabled; }

    void setThreshold(float db) noexcept
    {
        thresholdDb = db;
        thresholdLinear = juce::Decibels::decibelsToGain(db);
    }
    float getThreshold() const noexcept { return thresholdDb; }

    void setAttack(float ms) noexcept
    {
        attackMs = juce::jmax(0.1f, ms);
        updateTimeConstants();
    }
    float getAttack() const noexcept { return attackMs; }

    void setRelease(float ms) noexcept
    {
        releaseMs = juce::jmax(1.0f, ms);
        updateTimeConstants();
    }
    float getRelease() const noexcept { return releaseMs; }

    void setRange(float db) noexcept
    {
        rangeDb = juce::jmin(0.0f, db);
        rangeLinear = juce::Decibels::decibelsToGain(rangeDb);
    }
    float getRange() const noexcept { return rangeDb; }

    float getCurrentGainReduction() const noexcept { return gainReduction; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!enabled)
        {
            gainReduction = 1.0f;
            return;
        }

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            // Find max absolute level across all channels for sidechain detector
            float maxSample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                maxSample = std::max(maxSample, std::abs(buffer.getSample(ch, i)));
            }

            // Envelope follower
            if (maxSample > envelope)
                envelope += attackCoeff * (maxSample - envelope);
            else
                envelope += releaseCoeff * (maxSample - envelope);

            // Target gain based on threshold
            float targetGain = (envelope >= thresholdLinear) ? 1.0f : rangeLinear;

            // Smooth gain changes to avoid clicks
            gainReduction += (targetGain > gainReduction ? attackCoeff : releaseCoeff) * (targetGain - gainReduction);

            // Apply gain
            for (int ch = 0; ch < numChannels; ++ch)
            {
                buffer.setSample(ch, i, buffer.getSample(ch, i) * gainReduction);
            }
        }
    }

private:
    void updateTimeConstants()
    {
        if (currentSampleRate > 0.0)
        {
            attackCoeff  = 1.0f - std::exp(-1.0f / (float(currentSampleRate * 0.001) * attackMs));
            releaseCoeff = 1.0f - std::exp(-1.0f / (float(currentSampleRate * 0.001) * releaseMs));
        }
    }

    double currentSampleRate = 44100.0;
    bool enabled = true;

    float thresholdDb = -45.0f;
    float thresholdLinear = 0.0056f;
    float attackMs = 2.0f;
    float releaseMs = 120.0f;
    float rangeDb = -60.0f;
    float rangeLinear = 0.001f;

    float attackCoeff = 0.01f;
    float releaseCoeff = 0.001f;
    float envelope = 0.0f;
    float gainReduction = 1.0f;
};
