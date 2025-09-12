/*
27.500 -0.774
55.000 -0.568
110.000 -0.360
220.000 -0.205
440.000 -0.110
880.000 -0.057
1760.000 -0.029

*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

#include "AudioFile.h"

#include "impl/BiquadBandPass.h"
#include "impl/PingExcitation.h"

TEST(BiquadExcitationTest, amplitudeAndResponse)
{
    constexpr auto sampleRate{48000.f};
    constexpr auto periodLength = 100;
    constexpr auto testFrequency = sampleRate / periodLength; // 480 Hz
    constexpr auto patternLength = 1024;
    constexpr auto decayThreshold = 0.001f; // Stop measuring when max value drops below this

    BiquadBandPass bq;
    Excitation excitation(patternLength);

    excitation.generateSineWave();
    bq.setSampleRate(sampleRate);
    bq.setByDecay(testFrequency, 0.1f);

    // Calculate phase advance for the test frequency
    const float samplesForTwoPeriods = (2.0f / testFrequency) * sampleRate;
    const float phaseAdvance = static_cast<float>(patternLength) / samplesForTwoPeriods;

    // Process the excitation signal through the bandpass filter
    std::vector<float> output;
    float currentPhase = static_cast<float>(patternLength - 1); // Start from end
    float globalMin = 0.0f;
    float globalMax = 0.0f;

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
    std::cout << globalMin << "\t" << globalMax << std::endl;

    // Continue processing with zero input to capture full decay
    while (output.size() < maxSamples)
    {
        const float filterOutput = bq.step(0.0f);
        output.push_back(filterOutput);

        globalMin = std::min(globalMin, filterOutput);
        globalMax = std::max(globalMax, filterOutput);
    }

    // Find zero crossings
    std::vector<size_t> zeroCrossings;
    for (size_t i = 1; i < output.size(); ++i)
    {
        if (output[i - 1] < 0.0f && output[i] >= 0.0f)
        {
            zeroCrossings.push_back(i);
        }
    }

    // Measure decay time: find max absolute value between each zero crossing
    std::vector<float> peaksBetweenCrossings;
    size_t decaySteps = 0;

    for (size_t i = 0; i < zeroCrossings.size() - 1; ++i)
    {
        const size_t startIdx = zeroCrossings[i];
        const size_t endIdx = zeroCrossings[i + 1];

        float maxAbsInSegment = 0.0f;
        for (size_t j = startIdx; j < endIdx && j < output.size(); ++j)
        {
            maxAbsInSegment = std::max(maxAbsInSegment, std::abs(output[j]));
        }

        peaksBetweenCrossings.push_back(maxAbsInSegment);

        // Count steps until decay threshold is reached
        if (maxAbsInSegment >= decayThreshold)
        {
            decaySteps = endIdx;
        }
    }

    // Calculate average period length from zero crossings
    float averagePeriodLength = 0.0f;
    if (zeroCrossings.size() >= 2)
    {
        const size_t totalSamplesBetweenCrossings = zeroCrossings.back() - zeroCrossings.front();
        const size_t numberOfPeriods = zeroCrossings.size() - 1;
        averagePeriodLength = static_cast<float>(totalSamplesBetweenCrossings) / static_cast<float>(numberOfPeriods);
    }

    // Assertions for the interaction behavior
    EXPECT_GT(globalMax, 0.0f) << "Filter should produce positive peaks";
    EXPECT_LT(globalMin, 0.0f) << "Filter should produce negative peaks";
    EXPECT_GT(zeroCrossings.size(), 5) << "Should have multiple zero crossings for oscillation";
    EXPECT_GT(decaySteps, 0) << "Should measure decay over time";
    EXPECT_LT(decaySteps, output.size()) << "Should decay before reaching maximum samples";

    if (!peaksBetweenCrossings.empty())
    {
        // Verify decay: later peaks should generally be smaller than earlier ones
        const float firstPeak = peaksBetweenCrossings.front();
        const float lastPeak = peaksBetweenCrossings.back();
        EXPECT_GT(firstPeak, lastPeak) << "Filter response should decay over time";

        // Check that we have reasonable decay progression
        bool foundDecreasingTrend = false;
        for (size_t i = 1; i < peaksBetweenCrossings.size(); ++i)
        {
            if (peaksBetweenCrossings[i] < peaksBetweenCrossings[i - 1] * 0.9f) // 10% decay
            {
                foundDecreasingTrend = true;
                break;
            }
        }
        EXPECT_TRUE(foundDecreasingTrend) << "Should show clear decay trend";
    }

    // Verify period length is reasonable (should be close to expected frequency response)
    if (averagePeriodLength > 0.0f)
    {
        const float measuredFrequency = sampleRate / (averagePeriodLength * 2.0f); // *2 for full cycle
        const float frequencyRatio = measuredFrequency / testFrequency;
        EXPECT_GT(frequencyRatio, 0.8f) << "Measured frequency should be reasonably close to input";
        EXPECT_LT(frequencyRatio, 1.2f) << "Measured frequency should be reasonably close to input";
    }
}
float logisticCompensation(float frequency)
{
    const float y_inf = -0.00337839f;
    const float y0 = -1.157336f;
    const float fc = 52.56845f;
    const float k = 1.081108f;
    const float ratio = std::pow(frequency / fc, k);
    // This yields y in the range [y0 … y_inf]; invert sign if needed
    return y_inf + (y0 - y_inf) / (1.0f + ratio);
}

TEST(BiquadExcitationTest, frequencyAnalysis)
{
    constexpr auto sampleRate{48000.f};
    constexpr auto patternLength = 1024;
    constexpr auto decayThreshold = 0.001f;
    constexpr int startMidiNote = 21; // 27.5 Hz (A0)
    constexpr int endMidiNote = 108;  // 4186 Hz (C8)

    // Print CSV header
    std::cout << "MidiNote\tFrequency\tMinAfterExcitation\tMaxAfterExcitation\t"
              << "MinAfterDecay\tMaxAfterDecay\tMeasuredFrequency\tDecayTime" << std::endl;

    for (int midiNote = startMidiNote; midiNote <= endMidiNote; midiNote += 1)
    {
        // Calculate frequency from MIDI note number
        const float frequency = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);

        BiquadBandPass bq;
        Excitation excitation(patternLength);

        excitation.generateSineWave();
        // excitation.generateNoise();
        bq.setSampleRate(sampleRate);
        bq.setByDecay(frequency, 0.1f);

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
            const float compensation = 1 / logisticCompensation(frequency);
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
        std::cout << std::fixed << std::setprecision(3) << midiNote << "\t" << frequency << "\t" << minAfterExcitation
                  << "\t" << maxAfterExcitation << "\t" << minAfterDecay << "\t" << maxAfterDecay << "\t"
                  << measuredFrequency << "\t" << decaySteps << std::endl;
    }

    // This test always passes - it's just for data collection
    EXPECT_TRUE(true);
}