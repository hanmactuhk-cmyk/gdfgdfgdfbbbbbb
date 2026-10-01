#include "AutoKeyDetector.h"
#include <cmath>
#include <numeric>

static const char* sKeyNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// Krumhansl-Schmuckler Key Profiles
static const float kMajorProfile[12] = {
    6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
};

static const float kMinorProfile[12] = {
    6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f
};

AutoKeyDetector::AutoKeyDetector()
    : forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    fifo.fill(0.0f);
    fftData.fill(0.0f);
    chroma.fill(0.0f);
}

void AutoKeyDetector::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    reset();
}

void AutoKeyDetector::reset()
{
    fifoIndex = 0;
    analysisCounter = 0;
    fifo.fill(0.0f);
    fftData.fill(0.0f);
    chroma.fill(0.0f);
    confidence = 0.0f;
}

const char* AutoKeyDetector::getKeyName(int index)
{
    if (index >= 0 && index < 12)
        return sKeyNames[index];
    return "C";
}

void AutoKeyDetector::setManualKey(int keyIndex, bool isMajor)
{
    currentKeyIndex = juce::jlimit(0, 11, keyIndex);
    currentIsMajor = isMajor;
    confidence = 100.0f;
}

juce::String AutoKeyDetector::getCurrentKeyName() const
{
    juce::String name = getKeyName(currentKeyIndex);
    name += currentIsMajor ? " Major" : " Minor";
    return name;
}

void AutoKeyDetector::process(const juce::AudioBuffer<float>& buffer)
{
    if (!autoKeyEnabled)
        return;

    const int numSamples = buffer.getNumSamples();
    const auto* channelData = buffer.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        fifo[fifoIndex++] = channelData[i];
        if (fifoIndex >= fftSize)
        {
            fifoIndex = 0;
            analyzeBuffer();
        }
    }
}

void AutoKeyDetector::analyzeBuffer()
{
    std::copy(fifo.begin(), fifo.end(), fftData.begin());
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

    // Map FFT frequency bins to 12 chroma pitch classes (from 65 Hz to 2000 Hz)
    float binResolution = (float)currentSampleRate / (float)fftSize;
    int minBin = (int)(65.0f / binResolution);
    int maxBin = (int)(2000.0f / binResolution);
    maxBin = std::min(maxBin, fftSize / 2);

    std::array<float, 12> currentChroma{};
    currentChroma.fill(0.0f);

    for (int bin = minBin; bin < maxBin; ++bin)
    {
        float freq = (float)bin * binResolution;
        // Pitch to MIDI note number: 69 + 12 * log2(freq / 440)
        float midiNote = 69.0f + 12.0f * std::log2(freq / 440.0f);
        int pitchClass = ((int)std::round(midiNote)) % 12;
        if (pitchClass < 0) pitchClass += 12;

        float magnitude = fftData[(size_t)bin];
        currentChroma[(size_t)pitchClass] += magnitude;
    }

    // Leaky integrator for chroma profile
    for (int i = 0; i < 12; ++i)
    {
        chroma[(size_t)i] = 0.85f * chroma[(size_t)i] + 0.15f * currentChroma[(size_t)i];
    }

    if (++analysisCounter % 4 == 0)
    {
        estimateKeyFromChroma();
    }
}

void AutoKeyDetector::estimateKeyFromChroma()
{
    float chromaSum = std::accumulate(chroma.begin(), chroma.end(), 0.0f);
    if (chromaSum < 1e-4f)
    {
        confidence = 0.0f;
        return;
    }

    float bestCorr = -2.0f;
    int bestKey = 0;
    bool bestIsMajor = true;
    std::vector<float> allCorrs;

    auto computeCorrelation = [](const std::array<float, 12>& c, const float* profile, int shift) -> float
    {
        float meanC = 0.0f, meanP = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            meanC += c[(size_t)i];
            meanP += profile[(i - shift + 12) % 12];
        }
        meanC /= 12.0f;
        meanP /= 12.0f;

        float num = 0.0f, denC = 0.0f, denP = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            float diffC = c[(size_t)i] - meanC;
            float diffP = profile[(i - shift + 12) % 12] - meanP;
            num += diffC * diffP;
            denC += diffC * diffC;
            denP += diffP * diffP;
        }
        if (denC <= 1e-6f || denP <= 1e-6f) return 0.0f;
        return num / std::sqrt(denC * denP);
    };

    // Test 12 Major keys
    for (int k = 0; k < 12; ++k)
    {
        float r = computeCorrelation(chroma, kMajorProfile, k);
        allCorrs.push_back(r);
        if (r > bestCorr)
        {
            bestCorr = r;
            bestKey = k;
            bestIsMajor = true;
        }
    }

    // Test 12 Minor keys
    for (int k = 0; k < 12; ++k)
    {
        float r = computeCorrelation(chroma, kMinorProfile, k);
        allCorrs.push_back(r);
        if (r > bestCorr)
        {
            bestCorr = r;
            bestKey = k;
            bestIsMajor = false;
        }
    }

    std::sort(allCorrs.rbegin(), allCorrs.rend());
    float runnerUp = allCorrs.size() > 1 ? allCorrs[1] : 0.0f;
    float margin = std::max(0.0f, bestCorr - runnerUp);

    currentKeyIndex = bestKey;
    currentIsMajor = bestIsMajor;
    confidence = juce::jlimit(0.0f, 100.0f, (bestCorr * 50.0f + margin * 50.0f));
}
