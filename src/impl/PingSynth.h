#pragma once
/*
 * TODO:
 *   - different trigger kernels with interpolation for frequency dependency
 */
#include <array>
#include <cmath>
#include <numbers>
#include <random>

#include "Biquad.h"

template <size_t BlockSize>
class PingSynth
{
    static constexpr int minMidiNote{21};  // A-1
    static constexpr int maxMidiNote{120}; // C8
#ifdef NDEBUG
    static constexpr int stepsPerSemitone{77};
#else
    static constexpr int stepsPerSemitone{11};
#endif
    static constexpr size_t NumElements{(maxMidiNote - minMidiNote) * stepsPerSemitone + 1};

  public:
    explicit PingSynth(const float sampleRate)
        : m_sampleRate(sampleRate)
    {
        fillFrequencyTable();
        createSincFeed();
        for (auto& f : m_bq)
        {
            f.setSampleRate(sampleRate);
        }
        assignFrequencyAndDecay();
    }


    void setDecay(const float decay) noexcept
    {
        m_decay = decay;
        assignFrequencyAndDecay();
    }

    // spread triggers to nearby slots
    void setSpread(const float fdbk) noexcept
    {
        m_spread = fdbk;
    }
    // spread triggers to overtones odd position, i.e. *3 *5 *7 *9
    void setOddsOvertones(const float value) noexcept
    {
        m_odds = value;
    }
    // spread triggers to overtones even position, i.e. *2 *4 *6 *8
    void setEvenOvertones(const float value) noexcept
    {
        m_evens = value;
    }
    void setStretchedOvertones(const float value) noexcept
    {
        m_stretched = value;
    }
    // apply a skew to the factor used by Even and Off overtones, e.g.
    // at value = 0 the even overtones are 2,4,6,8
    // with value=-0.152 the overtones are 1.8, 3.6, 5.4, 7.2 (2^-0.152)
    void setSkewOddOvertones(const float value)
    {
        m_skewOdds = std::pow(2, value);
    }
    void setSkewEvenOvertones(const float value)
    {
        m_skewEvens = std::pow(2, value);
    }

    void setRandomSpread(const float value)
    {
        m_randomSpread = value;
    }

    void setRandomPower(const float value)
    {
        m_randomPower = value;
    }

    void addSpreads(const size_t index, const float power)
    {
        if (m_spread <= 0.0f)
        {
            return;
        }
        const float sigma = m_spread * 6.0f;
        const float twoSigmaSquared = 2.0f * sigma * sigma;

        // Calculate maximum distance based on spread
        const int maxDistance = static_cast<int>(m_spread * 10) + 1;

        for (int i = 1; i <= maxDistance; ++i)
        {
            const auto offset = (i % 2) == 1 ? (i + 1) >> 1 : -(i >> 1); // alternate left and right

            // Calculate Gaussian power based on distance
            const auto distance = static_cast<float>(std::abs(offset));
            const auto gaussianPower = std::exp(-(distance * distance) / twoSigmaSquared);
            const auto adjustedPower = power * gaussianPower;

            if (adjustedPower < 1.f)
            {
                break;
            }
            const auto targetIndex = static_cast<int>(index) + offset * stepsPerSemitone / 10;
            triggerSingleSlot(targetIndex, adjustedPower);
        }
    }

    void triggerSingleSlot(const size_t index, const float power) noexcept
    {
        if (index < m_triggerGain.size())
        {
            m_triggerGain[index] = power;
            m_trigger[index] = m_triggerPattern.size();
        }
    }

    void triggerSlots(const size_t index, const float power) noexcept
    {
        triggerSingleSlot(index, power);
        addSpreads(index, power);
        addOdds(index, power);
        addEvens(index, power);
        addStretched(index, power);
    }


    void triggerVoice(const size_t height, const float velocity) noexcept
    {
        if (height < minMidiNote || height > maxMidiNote)
        {
            return;
        }
        const size_t relHeight = height - minMidiNote;
        size_t baseIdx = relHeight * stepsPerSemitone;
        triggerSlots(baseIdx, velocity * 180.f * (m_decay + 0.01f));
        // triggerSlots(baseIdx + 7 * stepsPerSemitone, velocity * 40.f * m_decay);
        // triggerSlots(baseIdx - 5 * stepsPerSemitone, velocity * 40.f * m_decay);
    }

    void stopVoice(const size_t height, const float velocity) noexcept {}

    void processBlock(std::array<float, BlockSize>& out) noexcept
    {
        for (size_t i = 0; i < BlockSize; ++i)
        {
            float sum{0.f};
            for (size_t j = 0; j < m_bq.size(); ++j)
            {
                if (m_trigger[j])
                {
                    m_trigger[j]--;
                    sum += m_bq[j].step(m_triggerGain[j] * m_triggerPattern[m_trigger[j]]);
                }
                else
                {
                    sum += m_bq[j].step(0.f);
                }
            }
            out[i] = sum;
        }
    }

  private:
    void createWhiteNoiseBurst()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distribution(0.f, 1.0f);

        std::generate(m_triggerPattern.begin(), m_triggerPattern.end(),
                      [&]() mutable
                      {
                          const float white = distribution(gen);
                          return white;
                      });
    }

    void createSincFeed()
    {
        const auto numSamples = m_triggerPattern.size();
        const auto kernelMid = static_cast<float>(numSamples - 1) * 0.5f;
        const auto kernelWidth = static_cast<float>(numSamples) * 0.15f; // or choose based on your use-case

        enum WindowType
        {
            Hann,
            Blackman
        };

        auto windowValue = [numSamples](size_t i, WindowType type)
        {
            switch (type)
            {
                case Hann:
                    return 0.5f - 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                                                  (numSamples - 1)); // Hann
                case Blackman:
                    return 0.42f -
                           0.5f *
                               std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / (numSamples - 1)) +
                           0.08f * std::cos(4.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                                            (numSamples - 1)); // Blackman
            }
            return 1.0f;
        };

        for (std::size_t i = 0; i < numSamples; ++i)
        {
            const auto x = static_cast<float>(i) - kernelMid;
            const auto sincVal = (x == 0.0f) ? 1.0f
                                             : std::sin(std::numbers::pi_v<float> * x / kernelWidth) /
                                                   (std::numbers::pi_v<float> * x / kernelWidth);
            m_triggerPattern[i] = sincVal * windowValue(i, Hann);
        }
    }

    void fillFrequencyTable() noexcept
    {
        const auto baseFrequency = 440 * std::pow(2.f, static_cast<float>(minMidiNote - 69) / 12.f);
        constexpr auto slotsPerOctave = static_cast<float>(stepsPerSemitone) * 12;
        for (size_t j = 0; j < m_bq.size(); ++j)
        {
            const auto f = baseFrequency * std::pow(2.f, static_cast<float>(j) / slotsPerOctave);
            m_frequencies[j] = f;
        }
    }

    size_t getFrequencyIndex(const float targetFreq) const noexcept
    {
        const auto baseFrequency = 440.0f * std::pow(2.0f, static_cast<float>(minMidiNote - 69) / 12.0f);
        constexpr auto slotsPerOctave = static_cast<float>(stepsPerSemitone) * 12.0f;

        // Inverse of: f = baseFrequency * std::pow(2.f, j / slotsPerOctave)
        // j = log2(f / baseFrequency) * slotsPerOctave
        const auto exactIndex = std::log2(targetFreq / baseFrequency) * slotsPerOctave;
        const auto roundedIndex = static_cast<size_t>(std::round(exactIndex));

        // Clamp to valid range
        return std::clamp(roundedIndex, size_t{0}, m_frequencies.size() - 1);
    }

    void assignFrequencyAndDecay() noexcept
    {
        for (size_t j = 0; j < m_bq.size(); ++j)
        {
            m_bq[j].setByDecay(m_frequencies[j], 0.02f + m_decay * 10.f);
        }
    }

    template <bool IsOdd>
    void addOvertones(const size_t index, const float power)
    {
        const auto value = IsOdd ? m_odds : m_evens;
        const auto skew = IsOdd ? m_skewOdds : m_skewEvens;

        if (value <= 0.0f)
        {
            return;
        }

        const auto currentFreq = m_frequencies[index];
        const auto maxFreq = m_frequencies.back();
        const int maxOvertone = 20;
        for (int overtoneNum = 1; overtoneNum <= maxOvertone; ++overtoneNum)
        {
            const auto overtoneMultiplier = IsOdd ? (2 * overtoneNum + 1) * skew : (2 * overtoneNum) * skew;
            const auto overtoneFreq = currentFreq * overtoneMultiplier;

            if (overtoneFreq >= maxFreq)
            {
                break;
            }

            const auto targetIndex = getFrequencyIndex(overtoneFreq);
            if (targetIndex >= m_bq.size())
            {
                continue;
            }

            const auto overtonePosition = static_cast<float>(overtoneNum - 1) / (maxOvertone - 1);

            float overtonePower;
            if (value <= 0.5f)
            {
                const auto decayFactor = 1.0f - overtonePosition;
                const auto equalFactor = 1.0f;
                const auto blend = value * 2.0f;
                overtonePower = power * value * (decayFactor * (1.0f - blend) + equalFactor * blend);
            }
            else
            {
                const auto equalFactor = 1.0f;
                const auto increaseFactor = overtonePosition;
                const auto blend = (value - 0.5f) * 2.0f;
                overtonePower = power * 0.5f * (equalFactor * (1.0f - blend) + increaseFactor * blend);
            }
            std::cout << m_frequencies[targetIndex] << "\t" << overtonePower << std::endl;
            if (overtonePower > .1f)
            {
                triggerSingleSlot(targetIndex, overtonePower);
                addSpreads(targetIndex, overtonePower);
            }
        }
    }

    void addOdds(const size_t index, const float power)
    {
        addOvertones<true>(index, power);
    }

    void addEvens(const size_t index, const float power)
    {
        addOvertones<false>(index, power);
    }

    void addStretched(const size_t index, const float power)
    {
        if (m_stretched <= 0.0f)
        {
            return;
        }

        const auto currentFreq = m_frequencies[index];
        const auto maxFreq = m_frequencies.back();
        const int maxOvertone = 20;

        for (int overtoneNum = 2; overtoneNum <= maxOvertone; ++overtoneNum) // Start from 2nd harmonic
        {
            // Piano-like inharmonicity: f_n = f_0 * n * sqrt(1 + B * n^2)
            // Where B is the inharmonicity coefficient
            const auto B = m_stretched * 0.001f; // Scale factor for musical usefulness
            const auto stretchFactor = std::sqrt(1.0f + B * overtoneNum * overtoneNum);
            const auto overtoneFreq = currentFreq * overtoneNum * stretchFactor;

            if (overtoneFreq >= maxFreq)
            {
                break;
            }

            const auto targetIndex = getFrequencyIndex(overtoneFreq);
            if (targetIndex >= m_bq.size())
            {
                continue;
            }

            // Use same power distribution as other overtones
            const auto overtonePosition = static_cast<float>(overtoneNum - 2) / (maxOvertone - 2);

            float overtonePower;
            if (m_stretched <= 0.5f)
            {
                const auto decayFactor = 1.0f - overtonePosition;
                const auto equalFactor = 1.0f;
                const auto blend = m_stretched * 2.0f;
                overtonePower = power * m_stretched * (decayFactor * (1.0f - blend) + equalFactor * blend);
            }
            else
            {
                const auto equalFactor = 1.0f;
                const auto increaseFactor = overtonePosition;
                const auto blend = (m_stretched - 0.5f) * 2.0f;
                overtonePower = power * 0.5f * (equalFactor * (1.0f - blend) + increaseFactor * blend);
            }

            if (overtonePower > 0.1f)
            {
                triggerSingleSlot(targetIndex, overtonePower);
                addSpreads(targetIndex, overtonePower);
            }
        }
    }

    std::array<BiquadBandPass, NumElements> m_bq{};
    std::array<unsigned, NumElements> m_trigger{};
    std::array<float, NumElements> m_triggerGain{};
    std::array<float, NumElements> m_frequencies{};
    std::array<float, 100> m_triggerPattern;
    float m_sampleRate;
    float m_spread{0.f};
    float m_decay{0.1f};
    float m_odds{0.f};
    float m_skewOdds{0.f};
    float m_evens{0.f};
    float m_skewEvens{0.f};
    float m_stretched{0.f};
    float m_randomSpread{0.f};
    float m_randomPower{0.f};
};