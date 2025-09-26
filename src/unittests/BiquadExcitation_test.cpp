

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

#include "impl/AudioFile.h"
#include "Filters/BiquadResoBP.h"
#include "impl/PingExcitation.h"

TEST(BiquadExcitationTest, amplitudeAndResponse)
{
    for (auto periodLength = 25; periodLength < 2000; periodLength *= 2)
    {
        constexpr auto sampleRate{48000.f};

        const auto testFrequency = sampleRate / periodLength; // 480 Hz
        constexpr auto patternLength = 1024;
        constexpr auto decayThreshold = 0.001f;

        AbacDsp::BiquadResoBP bq{sampleRate};
        Excitation excitation(patternLength);

        bq.setByDecay(0, testFrequency, 0.1f);

        const auto samplesForTwoPeriods = 2.0f / testFrequency * sampleRate;
        const auto phaseAdvance = static_cast<float>(patternLength) / samplesForTwoPeriods;

        std::vector<float> output;
        auto currentPhase = static_cast<float>(patternLength - 1);
        auto globalMin = 0.0f;
        auto globalMax = 0.0f;

        // Process until excitation is exhausted or we hit max samples
        const size_t maxSamples = 20000; // Safety limit
        while (currentPhase >= 0.0f && output.size() < maxSamples)
        {
            const float excitationValue = excitation.getInterpolatedValue(currentPhase);
            const float filterOutput = bq.step(excitationValue);
            output.push_back(filterOutput);
            // Track global min/max
            globalMin = std::min(globalMin, filterOutput);
            globalMax = std::max(globalMax, filterOutput);
            currentPhase -= phaseAdvance;
        }

        // Continue processing with zero input to capture full decay
        while (output.size() < maxSamples)
        {
            const float filterOutput = bq.step(0.0f);
            output.push_back(filterOutput);

            globalMin = std::min(globalMin, filterOutput);
            globalMax = std::max(globalMax, filterOutput);
        }

        size_t first = 0;
        size_t last = 0;
        size_t cntZC = 0;
        auto maxAbsInSegment = std::max(std::abs(globalMax), std::abs(globalMin));
        auto previousMax = maxAbsInSegment;
        for (size_t i = 1; i < output.size(); ++i)
        {
            maxAbsInSegment = std::max(maxAbsInSegment, output[i - 1]);

            if (output[i - 1] < 0.0f && output[i] >= 0.0f)
            {
                if (first == 0)
                {
                    first = i;
                }
                else
                {
                    if (maxAbsInSegment < decayThreshold)
                    {
                        last = i;
                        break;
                    }
                }
                if (++cntZC > 3) // skip first 3 as the sound builds up
                {
                    EXPECT_GT(previousMax, maxAbsInSegment); // check for clean decay
                }
                previousMax = maxAbsInSegment;
                maxAbsInSegment = 0.f;
            }
        }

        float averagePeriodLength = 0.0f;
        if (cntZC >= 2)
        {
            const size_t totalSamplesBetweenCrossings = last - first;
            const size_t numberOfPeriods = cntZC;
            averagePeriodLength =
                static_cast<float>(totalSamplesBetweenCrossings) / static_cast<float>(numberOfPeriods);
        }
        EXPECT_GT(globalMax, 0.0f) << "Filter should produce positive peaks";
        EXPECT_LT(globalMin, 0.0f) << "Filter should produce negative peaks";

        EXPECT_GT(cntZC, 0) << "Should measure decay over time";
        EXPECT_LT(cntZC, output.size()) << "Should decay before reaching maximum samples";

        if (averagePeriodLength > 0.0f)
        {
            const float measuredFrequency = sampleRate / averagePeriodLength;
            const float frequencyRatio = measuredFrequency / testFrequency;
            EXPECT_GT(frequencyRatio, 0.8f) << "Measured frequency should be reasonably close to input";
            EXPECT_LT(frequencyRatio, 1.2f) << "Measured frequency should be reasonably close to input";
        }
    }
}

float logisticCompensation(float x)
{
    float power = std::pow(x / 95.18412f, 1.189401f);
    float numerator = 1.f * (1.f + power);
    float denominator = 0.8258689f + 0.006020447f * power;
    float result = numerator / denominator;
    return result;
    // return 0.008366906f + (0.8298868f - 0.008366906f) / (1 + std::pow(x / 93.56315f, 1.191096f));
    return 0.006020447f + (0.8258689f - 0.006020447f) / (1 + std::pow(x / 95.18412f, 1.189401f));
    // const float y_inf = -0.00337839f;
    // const float y0 = -1.157336f;
    // const float fc = 52.56845f;
    // const float k = 1.081108f;
    // const float ratio = std::pow(frequency / fc, k);
    // // This yields y in the range [y0 … y_inf]; invert sign if needed
    // return y_inf + (y0 - y_inf) / (1.0f + ratio);
}

TEST(BiquadExcitationTest, frequencyAnalysis)
{
    constexpr auto sampleRate{48000.f};
    constexpr auto patternLength = 1024;
    constexpr auto decayThreshold = 0.001f;
    constexpr int startMidiNote = 10; // 27.5 Hz (A0)
    constexpr int endMidiNote = 128;  // 4186 Hz (C8)

    // Print CSV header
    std::cout << "MidiNote\tFrequency\tMinDelta\tMinAfterExcitation\tMaxAfterExcitation\t"
              << "MeasuredFrequency\tDecayTime" << std::endl;

    for (int midiNote = startMidiNote; midiNote <= endMidiNote; midiNote += 1)
    {
        // Calculate frequency from MIDI note number
        const float frequency = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);

        AbacDsp::BiquadResoBP bq;
        Excitation excitation(patternLength);
        excitation.setNoise(0.0f);

        // excitation.generateNoise();
        bq.setSampleRate(sampleRate);
        bq.setByDecay(0, frequency, 0.1f);

        // Calculate phase advance for this frequency
        const float samplesForTwoPeriods = (2.0f / frequency) * sampleRate;
        const float phaseAdvance = static_cast<float>(patternLength) / samplesForTwoPeriods;

        std::vector<float> output;
        float currentPhase = static_cast<float>(patternLength - 1);

        // Phase 1: Process excitation signal
        float minAfterExcitation = 0.0f;
        float maxAfterExcitation = 0.0f;

        const size_t maxSamples = 20000;
        while (currentPhase >= 0.0f && output.size() < maxSamples)
        {
            const float excitationValue = excitation.getInterpolatedValue(currentPhase);
            // const float compensation = 12.0f * std::pow(frequency / 440.0f, 0.7f);
            const float compensation = 0.5f * logisticCompensation(frequency);
            // const float compensation = 1.f;
            const float filterOutput = bq.step(excitationValue * compensation);
            output.push_back(filterOutput);

            minAfterExcitation = std::min(minAfterExcitation, filterOutput);
            maxAfterExcitation = std::max(maxAfterExcitation, filterOutput);

            currentPhase -= phaseAdvance;
        }

        // Phase 2: Continue with zero input and track decay with corrected algorithm
        float minAfterDecay = minAfterExcitation;
        float maxAfterDecay = maxAfterExcitation;

        float previousSample = output.empty() ? 0.0f : output.back();
        float localAbsMax = 0.0f;
        size_t zeroCrossingCount = 0;
        size_t firstZeroCrossingPos = 0;
        size_t lastZeroCrossingPos = 0;
        size_t decaySteps = output.size();
        bool foundFirstZeroCrossing = false;

        while (output.size() < maxSamples)
        {
            const float filterOutput = bq.step(0.0f);
            output.push_back(filterOutput);

            minAfterDecay = std::min(minAfterDecay, filterOutput);
            maxAfterDecay = std::max(maxAfterDecay, filterOutput);

            localAbsMax = std::max(localAbsMax, std::abs(filterOutput));

            if (previousSample < 0.0f && filterOutput >= 0.0f)
            {
                if (!foundFirstZeroCrossing)
                {
                    firstZeroCrossingPos = output.size() - 1;
                    foundFirstZeroCrossing = true;
                }

                zeroCrossingCount++;
                lastZeroCrossingPos = output.size() - 1;
                if (localAbsMax < decayThreshold)
                {
                    decaySteps = output.size();
                    break;
                }
                localAbsMax = 0.0f;
            }

            previousSample = filterOutput;
        }

        // Calculate measured frequency from zero crossings
        float measuredFrequency = 0.0f;
        if (zeroCrossingCount >= 2 && foundFirstZeroCrossing)
        {
            const size_t totalSamplesBetweenCrossings = lastZeroCrossingPos - firstZeroCrossingPos;
            const float averagePeriodLength =
                static_cast<float>(totalSamplesBetweenCrossings) / static_cast<float>(zeroCrossingCount - 1);
            measuredFrequency = sampleRate / averagePeriodLength; // *2 for full cycle
        }

        // Save output as WAV file
        AudioFile<float> audioFile;
        audioFile.setAudioBuffer({{output}}); // Mono channel
        audioFile.setSampleRate(static_cast<int>(sampleRate));
        audioFile.setBitDepth(32); // 32-bit float

        char filename[64];
        std::snprintf(filename, sizeof(filename), "/tmp/note_%03d.wav", midiNote);
        audioFile.save(filename);

        // Output results as tab-separated values
        // std::cout << std::fixed << std::setprecision(3) << midiNote << "\t" << frequency << "\t"
        //           << (maxAfterExcitation - minAfterExcitation) * 0.5f << "\t" << minAfterExcitation << "\t"
        //           << maxAfterExcitation << "\t" << measuredFrequency << "\t" << decaySteps << std::endl;
        std::cout << frequency << "\t" << std::log10(0.5f * (maxAfterExcitation - minAfterExcitation)) * 20.f
                  << std::endl;
    }

    // This test always passes - it's just for data collection
    EXPECT_TRUE(true);
}