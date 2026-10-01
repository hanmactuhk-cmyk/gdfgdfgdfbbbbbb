#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

class ParametricEQ
{
public:
    struct Band
    {
        float frequency = 1000.0f;
        float gainDb = 0.0f;
        float q = 1.0f;
        bool enabled = true;
    };

    static constexpr int NumBands = 13;
    static constexpr std::array<float, NumBands> DefaultFrequencies = {
        20.0f, 50.0f, 80.0f, 100.0f, 200.0f, 500.0f, 1000.0f,
        2000.0f, 5000.0f, 8000.0f, 10000.0f, 15000.0f, 20000.0f
    };

    ParametricEQ()
    {
        for (int i = 0; i < NumBands; ++i)
        {
            bands[i].frequency = DefaultFrequencies[i];
            bands[i].gainDb = 0.0f;
            bands[i].q = 1.0f;
            bands[i].enabled = true;
        }
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;

        for (int i = 0; i < NumBands; ++i)
        {
            filters[i].prepare(spec);
            filters[i].reset();
            updateBandFilter(i);
        }
    }

    void setEnabled(bool shouldBeEnabled) noexcept { globalEnabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return globalEnabled; }

    void setBand(int index, float freqHz, float gainDb, float qVal, bool active = true)
    {
        if (index >= 0 && index < NumBands)
        {
            bands[index].frequency = juce::jlimit(20.0f, 22000.0f, freqHz);
            bands[index].gainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
            bands[index].q = juce::jlimit(0.1f, 12.0f, qVal);
            bands[index].enabled = active;
            updateBandFilter(index);
        }
    }

    Band getBand(int index) const noexcept
    {
        if (index >= 0 && index < NumBands)
            return bands[index];
        return {};
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!globalEnabled)
            return;

        const int numChannels = buffer.getNumChannels();
        if (numChannels == 0)
            return;

        juce::dsp::AudioBlock<float> block(buffer);

        for (int i = 0; i < NumBands; ++i)
        {
            if (bands[i].enabled && std::abs(bands[i].gainDb) > 0.01f)
            {
                juce::dsp::ProcessContextReplacing<float> context(block);
                filters[i].process(context);
            }
        }
    }

private:
    void updateBandFilter(int index)
    {
        if (currentSampleRate <= 0.0)
            return;

        const auto& b = bands[index];
        const float nyquist = float(currentSampleRate * 0.499);
        const float safeFreq = juce::jlimit(20.0f, nyquist, b.frequency);
        const float gainLinear = juce::Decibels::decibelsToGain(b.gainDb);

        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

        if (index == 0)
        {
            // Low Shelf for sub bass 20 Hz
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, safeFreq, b.q, gainLinear);
        }
        else if (index == NumBands - 1)
        {
            // High Shelf for high air 20 kHz
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, safeFreq, b.q, gainLinear);
        }
        else
        {
            // Parametric Peak filter
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, safeFreq, b.q, gainLinear);
        }

        if (coeffs != nullptr)
        {
            *filters[index].state = *coeffs;
        }
    }

    double currentSampleRate = 44100.0;
    bool globalEnabled = true;

    std::array<Band, NumBands> bands;
    std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, NumBands> filters;
};
