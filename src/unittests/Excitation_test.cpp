#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <cmath>
#include <numbers>

#include "impl/PingExcitation.h"

TEST(ExcitationTest, generalFunctionality)
{
    constexpr auto sampleRate{48000.f};
    constexpr auto periodLength = 100;
    constexpr auto testFrequency = sampleRate / periodLength; // 480 Hz
    constexpr auto patternLength = 1024;

    Excitation excitation(patternLength);

    // Calculate phase advance for the test frequency
    // Duration of 2 periods at testFrequency
    const float samplesForTwoPeriods = (2.0f / testFrequency) * sampleRate;
    const float phaseAdvance = static_cast<float>(patternLength) / samplesForTwoPeriods;
    std::vector<float> output;
    output.reserve(static_cast<size_t>(samplesForTwoPeriods * 2));

    float currentPhase = static_cast<float>(patternLength - 1);

    while (currentPhase >= 0.0f && output.size() < output.capacity())
    {
        const float excitationValue = excitation.getInterpolatedValue(currentPhase);
        output.push_back(excitationValue);
        currentPhase -= phaseAdvance;
    }
    EXPECT_NEAR(output[35], -0.219f, 1E-1f);
    EXPECT_NEAR(output[50], 0, 1E-1f);
    EXPECT_NEAR(output[78], 0.8682f, 1E-1f);
    EXPECT_NEAR(output[100], 0, 1E-1f);
    EXPECT_NEAR(output[122], -0.87035f, 1E-1f);
    EXPECT_NEAR(output[150], 0, 1E-1f);
    EXPECT_NEAR(output[164], 0.2217, 1E-1f);
}