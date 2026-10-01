#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class DeEsser
{
public:
    DeEsser() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        sidechainBandpass.prepare(spec);
        dynamicNotchFilter.prepare(spec);

        updateFilters();
        envelope = 0.0f;
        gainReduction = 1.0f;
    }

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return enabled; }

    void setFrequency(float freqHz) noexcept
    {
        frequency = juce::jlimit(2500.0f, 12000.0f, freqHz);
        updateFilters();
    }
    float getFrequency() const noexcept { return frequency; }

    void setThreshold(float db) noexcept
    {
        thresholdDb = db;
        thresholdLinear = juce::Decibels::decibelsToGain(db);
    }
    float getThreshold() const noexcept { return thresholdDb; }

    void setAmount(float amt) noexcept
    {
        amount = juce::jlimit(0.0f, 1.0f, amt);
    }
    float getAmount() const noexcept { return amount; }

    float getCurrentReduction() const noexcept { return gainReduction; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!enabled || amount <= 0.001f)
        {
            gainReduction = 1.0f;
            return;
        }

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Copy buffer to analyze sidechain bandpass
        sidechainBuffer.makeCopyOf(buffer);
        juce::dsp::AudioBlock<float> sidechainBlock(sidechainBuffer);
        juce::dsp::ProcessContextReplacing<float> scContext(sidechainBlock);
        sidechainBandpass.process(scContext);

        const float attackCoeff = 0.05f;
        const float releaseCoeff = 0.002f;

        for (int i = 0; i < numSamples; ++i)
        {
            float scMax = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                scMax = std::max(scMax, std::abs(sidechainBuffer.getSample(ch, i)));
            }

            if (scMax > envelope)
                envelope += attackCoeff * (scMax - envelope);
            else
                envelope += releaseCoeff * (scMax - envelope);

            float targetReduction = 1.0f;
            if (envelope > thresholdLinear && thresholdLinear > 1e-6f)
            {
                float excess = envelope / thresholdLinear;
                // Reduction proportional to amount
                float maxAttenuationDb = -24.0f * amount;
                float currentAttDb = juce::jmax(maxAttenuationDb, -6.0f * (excess - 1.0f) * amount);
                targetReduction = juce::Decibels::decibelsToGain(currentAttDb);
            }

            gainReduction += 0.05f * (targetReduction - gainReduction);

            // Apply notch cut to sibilant high frequencies
            for (int ch = 0; ch < numChannels; ++ch)
            {
                // Dynamic attenuation on sibilance
                float s = buffer.getSample(ch, i);
                float scSample = sidechainBuffer.getSample(ch, i);
                // Subtract attenuated bandpass portion
                buffer.setSample(ch, i, s - (1.0f - gainReduction) * scSample);
            }
        }
    }

private:
    void updateFilters()
    {
        if (currentSampleRate <= 0.0)
            return;

        auto bpCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(currentSampleRate, frequency, 2.5f);
        if (bpCoeffs != nullptr)
            *sidechainBandpass.state = *bpCoeffs;
    }

    double currentSampleRate = 44100.0;
    bool enabled = true;

    float frequency = 6500.0f;
    float thresholdDb = -24.0f;
    float thresholdLinear = 0.063f;
    float amount = 0.5f;

    float envelope = 0.0f;
    float gainReduction = 1.0f;

    juce::AudioBuffer<float> sidechainBuffer;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> sidechainBandpass;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> dynamicNotchFilter;
};
