#include "DspChain.h"

DspChain::DspChain()
{
}

void DspChain::prepare(double sampleRate, int samplesPerBlock)
{
    noiseGate.prepare(sampleRate, samplesPerBlock);
    compressor.prepare(sampleRate, samplesPerBlock);
    parametricEQ.prepare(sampleRate, samplesPerBlock);
    deEsser.prepare(sampleRate, samplesPerBlock);
    reverb.prepare(sampleRate, samplesPerBlock);
    limiter.prepare(sampleRate, samplesPerBlock);
}

void DspChain::reset()
{
}

void DspChain::process(juce::AudioBuffer<float>& buffer)
{
    // Exact requested routing for MIC BUS:
    // MIC INPUT -> Noise Gate -> Compressor -> EQ -> De-Esser -> Reverb
    noiseGate.process(buffer);
    compressor.process(buffer);
    parametricEQ.process(buffer);
    deEsser.process(buffer);
    reverb.process(buffer);
}
