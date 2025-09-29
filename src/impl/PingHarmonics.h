#pragma once

#include <functional>
#include <array>

template <size_t NumElements>
class HarmonicGeneratorBase
{
  public:
    using TriggerCallback = std::function<void(size_t, float, float)>;
    using SpreadCallback = std::function<void(size_t, float)>;

    explicit HarmonicGeneratorBase(const std::array<float, NumElements>& frequencies,
                                   const std::function<size_t(float)>& getFrequencyIndex,
                                   const std::function<float()>& getHumanRandomness, float& currentVelocity,
                                   float randomPower, TriggerCallback triggerCallback, SpreadCallback spreadCallback)
        : m_frequencies(frequencies)
        , m_getFrequencyIndex(getFrequencyIndex)
        , m_getHumanRandomness(getHumanRandomness)
        , m_currentVelocity(currentVelocity)
        , m_randomPower(randomPower)
        , m_triggerCallback(std::move(triggerCallback))
        , m_spreadCallback(std::move(spreadCallback))
    {
    }
    void setMinMaxOvertone(const std::pair<int, int>& overtoneCount)
    {
        m_overtoneCount = overtoneCount;
    }

    virtual ~HarmonicGeneratorBase() = default;
    virtual void generateHarmonics(size_t index, float power) = 0;

  protected:
    const std::array<float, NumElements>& m_frequencies;
    const std::function<size_t(float)>& m_getFrequencyIndex;
    const std::function<float()>& m_getHumanRandomness;
    float& m_currentVelocity;
    float m_randomPower;
    TriggerCallback m_triggerCallback;
    SpreadCallback m_spreadCallback;
    std::pair<int, int> m_overtoneCount{3, 10};

    void triggerHarmonic(const size_t targetIndex, const float overtonePower, const int order) noexcept
    {
        if (overtonePower > 0.001f)
        {
            m_triggerCallback(targetIndex, overtonePower,
                              static_cast<float>(order) / static_cast<float>(getMaxOvertone()));
            if (order < m_overtoneCount.first)
            {
                m_spreadCallback(targetIndex, overtonePower);
            }
        }
    }

    [[nodiscard]] float applyPowerRandomness(const float power) const noexcept
    {
        if (m_randomPower > 0.0f)
        {
            const auto powerVariation = 1.0f + m_getHumanRandomness() * m_randomPower * 0.3f;
            return power * powerVariation;
        }
        return power;
    }

    [[nodiscard]] int getMaxOvertone() const noexcept
    {
        return static_cast<int>(m_overtoneCount.first +
                                m_currentVelocity * (m_overtoneCount.second - m_overtoneCount.first));
    }

    [[nodiscard]] static float calculateOvertonePower(const float basePower, const float value,
                                                      const float overtonePosition) noexcept
    {
        float p;
        if (value <= 0.5f)
        {
            const auto decayFactor = 1.0f - overtonePosition;
            const auto blend = value * 2.0f;
            p= basePower * value * (decayFactor * (1.0f - blend) + blend);
        }else
        {
            const auto increaseFactor = overtonePosition;
            const auto blend = value * 2.f - 1.f;
            p= basePower * 0.5f * (1.f - blend + increaseFactor * blend);
        }
        return p*p*p;
    }
};

template <size_t NumElements>
class OddHarmonicGenerator final : public HarmonicGeneratorBase<NumElements>
{
  private:
    float m_odds{0.0f};
    float m_skewOdds{1.0f};

  public:
    using Base = HarmonicGeneratorBase<NumElements>;

    explicit OddHarmonicGenerator(const std::array<float, NumElements>& frequencies,
                                  const std::function<size_t(float)>& getFrequencyIndex,
                                  const std::function<float()>& getHumanRandomness, float& currentVelocity,
                                  float randomPower, typename Base::TriggerCallback triggerCallback,
                                  typename Base::SpreadCallback spreadCallback)
        : Base(frequencies, getFrequencyIndex, getHumanRandomness, currentVelocity, randomPower,
               std::move(triggerCallback), std::move(spreadCallback))
    {
    }

    void setOdds(const float value) noexcept
    {
        m_odds = value;
    }

    void setSkewOdds(const float value) noexcept
    {
        m_skewOdds = std::pow(2.0f, value);
    }

    void generateHarmonics(const size_t index, const float power) override
    {
        if (m_odds <= 0.0f)
        {
            return;
        }

        const auto currentFreq = this->m_frequencies[index];
        const auto maxFreq = this->m_frequencies.back();
        const int maxOvertone = this->getMaxOvertone();

        for (int overtoneNum = 1; overtoneNum <= maxOvertone; ++overtoneNum)
        {
            const auto overtoneMultiplier = (2 * overtoneNum + 1) * m_skewOdds;
            const auto overtoneFreq = currentFreq * overtoneMultiplier;
            if (overtoneFreq >= maxFreq)
            {
                break;
            }
            const auto targetIndex = this->m_getFrequencyIndex(overtoneFreq);
            const auto overtonePosition = static_cast<float>(overtoneNum - 1) / (maxOvertone - 1);
            const auto overtonePower =
                this->applyPowerRandomness(this->calculateOvertonePower(power, m_odds, overtonePosition));

            this->triggerHarmonic(targetIndex, overtonePower, overtoneNum);
        }
    }
};

template <size_t NumElements>
class EvenHarmonicGenerator final : public HarmonicGeneratorBase<NumElements>
{
  private:
    float m_evens{0.0f};
    float m_skewEvens{1.0f};

  public:
    using Base = HarmonicGeneratorBase<NumElements>;

    explicit EvenHarmonicGenerator(const std::array<float, NumElements>& frequencies,
                                   const std::function<size_t(float)>& getFrequencyIndex,
                                   const std::function<float()>& getHumanRandomness, float& currentVelocity,
                                   float randomPower, typename Base::TriggerCallback triggerCallback,
                                   typename Base::SpreadCallback spreadCallback)
        : Base(frequencies, getFrequencyIndex, getHumanRandomness, currentVelocity, randomPower,
               std::move(triggerCallback), std::move(spreadCallback))
    {
    }

    void setEvens(const float value) noexcept
    {
        m_evens = value;
    }

    void setSkewEvens(const float value) noexcept
    {
        m_skewEvens = std::pow(2.0f, value);
    }

    void generateHarmonics(const size_t index, const float power) override
    {
        if (m_evens <= 0.0f)
        {
            return;
        }
        const auto currentFreq = this->m_frequencies[index];
        const auto maxFreq = this->m_frequencies.back();
        const int maxOvertone = this->getMaxOvertone();

        for (int overtoneNum = 1; overtoneNum <= maxOvertone; ++overtoneNum)
        {
            const auto overtoneMultiplier = static_cast<float>(2 * overtoneNum) * m_skewEvens;
            const auto overtoneFreq = currentFreq * overtoneMultiplier;
            if (overtoneFreq >= maxFreq)
            {
                break;
            }
            const auto targetIndex = this->m_getFrequencyIndex(overtoneFreq);
            const auto overtonePosition = static_cast<float>(overtoneNum - 1) / (maxOvertone - 1);
            auto overtonePower =
                this->applyPowerRandomness(this->calculateOvertonePower(power, m_evens, overtonePosition));
            this->triggerHarmonic(targetIndex, overtonePower, overtoneNum);
        }
    }
};

template <size_t NumElements>
class StretchedHarmonicGenerator final : public HarmonicGeneratorBase<NumElements>
{
  private:
    float m_stretched{0.0f};

  public:
    using Base = HarmonicGeneratorBase<NumElements>;

    explicit StretchedHarmonicGenerator(const std::array<float, NumElements>& frequencies,
                                        const std::function<size_t(float)>& getFrequencyIndex,
                                        const std::function<float()>& getHumanRandomness, float& currentVelocity,
                                        float randomPower, typename Base::TriggerCallback triggerCallback,
                                        typename Base::SpreadCallback spreadCallback)
        : Base(frequencies, getFrequencyIndex, getHumanRandomness, currentVelocity, randomPower,
               std::move(triggerCallback), std::move(spreadCallback))
    {
    }

    void setStretched(const float value) noexcept
    {
        m_stretched = value;
    }

    void generateHarmonics(const size_t index, const float power) override
    {
        if (m_stretched <= 0.0f)
        {
            return;
        }

        const auto currentFreq = this->m_frequencies[index];
        const auto maxFreq = this->m_frequencies.back();
        const int maxOvertone = this->getMaxOvertone();

        for (int overtoneNum = 2; overtoneNum <= maxOvertone; ++overtoneNum)
        {
            // Piano-like inharmonicity: f_n = f_0 * n * sqrt(1 + B * n^2)
            const auto B = m_stretched * 0.01f;
            const auto stretchFactor = std::sqrt(1.0f + B * overtoneNum * overtoneNum);
            const auto overtoneFreq = currentFreq * overtoneNum * stretchFactor;
            if (overtoneFreq >= maxFreq)
            {
                break;
            }
            const auto targetIndex = this->m_getFrequencyIndex(overtoneFreq);
            const auto overtonePosition = static_cast<float>(overtoneNum - 2) / (maxOvertone - 2);
            auto overtonePower =
                this->applyPowerRandomness(this->calculateOvertonePower(power, m_stretched, overtonePosition));
            this->triggerHarmonic(targetIndex, overtonePower, overtoneNum - 1);
        }
    }
};