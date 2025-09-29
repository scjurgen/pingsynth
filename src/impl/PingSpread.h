#pragma once

#include <array>
#include <functional>
#include <cmath>
#include <algorithm>

template <size_t N, int stepsPerSemitone>
class PingSpread
{
  public:
    using TriggerCallback = std::function<void(size_t, float, size_t)>;

    explicit PingSpread(const std::array<float, N>& frequencies, const std::function<size_t(float)>& getFrequencyIndex,
                        const std::function<float()>& getHumanRandomness, TriggerCallback triggerCallback)
        : m_frequencies(frequencies)
        , m_getFrequencyIndex(getFrequencyIndex)
        , m_getHumanRandomness(getHumanRandomness)
        , m_triggerCallback(std::move(triggerCallback))
    {
    }

    void setSpread(float spread) noexcept
    {
        m_spread = spread;
    }

    void setRandomSpread(float randomSpread) noexcept
    {
        m_randomSpread = randomSpread;
    }

    void setRandomPower(float randomPower) noexcept
    {
        m_randomPower = randomPower;
    }

    size_t beatingIndex(size_t index, const float f)
    {
        float mainF = m_frequencies[index];
        auto nIndex = m_getFrequencyIndex(mainF + f);
        // return 1;
        return 1+nIndex - index;
    }

    float getRandomSpread()
    {
        std::uniform_real_distribution uniform(0.0f, 1.0f);
        const float v = uniform(m_rng);
        return v * v * m_randomSpread * 3.f;
    }

    void generateSpreads(const size_t index, const float power)
    {
        if (m_spread <= 0.0f)
        {
            return;
        }
        const size_t beatDelta = beatingIndex(index, 4.f);
        if (m_spread < 0.5f)
        {
            const auto randomOffset = getRandomSpread()*beatDelta*0.5f;
            std::cout << beatDelta << "\t" << randomOffset << std::endl;
            const auto powerVariation =
                m_randomPower > 0.0f ? 1.0f + m_getHumanRandomness() * m_randomPower * 0.5f : 1.0f;
            const auto adjustedPower = m_spread * 2 * power * powerVariation;
            const auto targetIndex = static_cast<size_t>(index + beatDelta + randomOffset);
            m_triggerCallback(targetIndex, adjustedPower, 1);
        }
        else
        {
            {
                const auto randomOffset = getRandomSpread()*beatDelta*0.5f;
            std::cout << beatDelta << "\t" << randomOffset << std::endl;
                const auto targetIndex = static_cast<size_t>(index + beatDelta + randomOffset);
                m_triggerCallback(targetIndex, power, 1);
            }
            {
                const auto randomOffset = getRandomSpread()*beatDelta*0.5f;
                std::cout << beatDelta << "\t" << randomOffset << std::endl;
                const auto powerVariation =
                    m_randomPower > 0.0f ? 1.0f + m_getHumanRandomness() * m_randomPower * 0.5f : 1.0f;
                const auto adjustedPower = (m_spread - 0.5f) * 2 * power * powerVariation;
                const auto targetIndex = static_cast<size_t>(index - beatDelta - randomOffset);
                m_triggerCallback(targetIndex, adjustedPower, 1);
            }
        }
    }

  private:
    const std::array<float, N>& m_frequencies;
    const std::function<size_t(float)>& m_getFrequencyIndex;
    const std::function<float()>& m_getHumanRandomness;
    TriggerCallback m_triggerCallback;
    float m_spread{0.0f};
    float m_randomSpread{0.0f};
    float m_randomPower{0.0f};
    mutable std::mt19937 m_rng{std::random_device{}()};
};