#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class VocalCompressor
{
public:
    VocalCompressor() = default;

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        updateTimeConstants();
        envelopeDb = -96.0f;
        gainReductionDb = 0.0f;
    }

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return enabled; }

    void setThreshold(float db) noexcept { thresholdDb = db; }
    float getThreshold() const noexcept { return thresholdDb; }

    void setRatio(float r) noexcept { ratio = juce::jmax(1.0f, r); }
    float getRatio() const noexcept { return ratio; }

    void setAttack(float ms) noexcept
    {
        attackMs = juce::jmax(0.1f, ms);
        updateTimeConstants();
    }
    float getAttack() const noexcept { return attackMs; }

    void setRelease(float ms) noexcept
    {
        releaseMs = juce::jmax(5.0f, ms);
        updateTimeConstants();
    }
    float getRelease() const noexcept { return releaseMs; }

    void setMakeupGain(float db) noexcept
    {
        makeupGainDb = db;
        makeupLinear = juce::Decibels::decibelsToGain(db);
    }
    float getMakeupGain() const noexcept { return makeupGainDb; }

    float getCurrentGainReductionDb() const noexcept { return gainReductionDb; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!enabled)
        {
            gainReductionDb = 0.0f;
            return;
        }

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            // Peak sidechain
            float maxSample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                maxSample = std::max(maxSample, std::abs(buffer.getSample(ch, i)));
            }

            // Convert to dB
            float inputDb = (maxSample > 1e-5f) ? juce::Decibels::gainToDecibels(maxSample) : -100.0f;

            // Attack/Release on envelope
            if (inputDb > envelopeDb)
                envelopeDb += attackCoeff * (inputDb - envelopeDb);
            else
                envelopeDb += releaseCoeff * (inputDb - envelopeDb);

            // Compute compression
            float targetGainReduction = 0.0f;
            if (envelopeDb > thresholdDb)
            {
                // Amount over threshold compressed by ratio
                float excess = envelopeDb - thresholdDb;
                targetGainReduction = excess * (1.0f - (1.0f / ratio));
            }

            gainReductionDb = targetGainReduction;
            float finalGain = juce::Decibels::decibelsToGain(-gainReductionDb) * makeupLinear;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                buffer.setSample(ch, i, buffer.getSample(ch, i) * finalGain);
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

    float thresholdDb = -18.0f;
    float ratio = 3.5f;
    float attackMs = 15.0f;
    float releaseMs = 140.0f;
    float makeupGainDb = 3.0f;
    float makeupLinear = 1.4125f;

    float attackCoeff = 0.01f;
    float releaseCoeff = 0.001f;
    float envelopeDb = -96.0f;
    float gainReductionDb = 0.0f;
};
