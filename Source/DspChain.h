#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "NoiseGate.h"
#include "Compressor.h"
#include "ParametricEQ.h"
#include "DeEsser.h"
#include "ReverbProcessor.h"
#include "LimiterProcessor.h"

class DspChain
{
public:
    DspChain();
    ~DspChain() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void process(juce::AudioBuffer<float>& buffer);

    // Modules
    NoiseGate& getNoiseGate() noexcept { return noiseGate; }
    VocalCompressor& getCompressor() noexcept { return compressor; }
    ParametricEQ& getEQ() noexcept { return parametricEQ; }
    DeEsser& getDeEsser() noexcept { return deEsser; }
    ReverbProcessor& getReverb() noexcept { return reverb; }
    LimiterProcessor& getLimiter() noexcept { return limiter; }

private:
    NoiseGate noiseGate;
    VocalCompressor compressor;
    ParametricEQ parametricEQ;
    DeEsser deEsser;
    ReverbProcessor reverb;
    LimiterProcessor limiter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspChain)
};
