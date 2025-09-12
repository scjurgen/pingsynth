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

    void triggerHarmonic(const size_t targetIndex, const float overtonePower, const int order)
    {
        if (overtonePower > 0.1f)
        {
            m_triggerCallback(targetIndex, overtonePower,
                              static_cast<float>(order) / static_cast<float>(getMaxOvertone()));
            if (order == 1)
            {
                m_spreadCallback(targetIndex, overtonePower);
            }
        }
    }

    float applyPowerRandomness(float power) const
    {
        if (m_randomPower > 0.0f)
        {
            const auto powerVariation = 1.0f + m_getHumanRandomness() * m_randomPower * 0.3f;
            return power * powerVariation;
        }
        return power;
    }

    int getMaxOvertone() const
    {
        return static_cast<int>(m_overtoneCount.first +
                                m_currentVelocity * (m_overtoneCount.second - m_overtoneCount.first));
    }

    float calculateOvertonePower(float basePower, float value, float overtonePosition) const
    {
        if (value <= 0.5f)
        {
            const auto decayFactor = 1.0f - overtonePosition;
            constexpr auto equalFactor = 1.0f;
            const auto blend = value * 2.0f;
            return basePower * value * (decayFactor * (1.0f - blend) + equalFactor * blend);
        }

        constexpr auto equalFactor = 1.0f;
        const auto increaseFactor = overtonePosition;
        const auto blend = (value - 0.5f) * 2.0f;
        return basePower * 0.5f * (equalFactor * (1.0f - blend) + increaseFactor * blend);
    }
};

template <size_t NumElements>
class OddHarmonicGenerator : public HarmonicGeneratorBase<NumElements>
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

    void setOdds(float value) noexcept
    {
        m_odds = value;
    }
    void setSkewOdds(float value) noexcept
    {
        m_skewOdds = std::pow(2.0f, value);
    }

    void generateHarmonics(size_t index, float power) override
    {
        if (m_odds <= 0.0f)
            return;

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
            if (targetIndex >= this->m_frequencies.size())
            {
                continue;
            }

            const auto overtonePosition = static_cast<float>(overtoneNum - 1) / (maxOvertone - 1);
            auto overtonePower = this->calculateOvertonePower(power, m_odds, overtonePosition);
            overtonePower = this->applyPowerRandomness(overtonePower);

            this->triggerHarmonic(targetIndex, overtonePower, overtoneNum);
        }
    }
};

template <size_t NumElements>
class EvenHarmonicGenerator : public HarmonicGeneratorBase<NumElements>
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

    void setEvens(float value) noexcept
    {
        m_evens = value;
    }
    void setSkewEvens(float value) noexcept
    {
        m_skewEvens = std::pow(2.0f, value);
    }

    void generateHarmonics(size_t index, float power) override
    {
        if (m_evens <= 0.0f)
            return;

        const auto currentFreq = this->m_frequencies[index];
        const auto maxFreq = this->m_frequencies.back();
        const int maxOvertone = this->getMaxOvertone();

        for (int overtoneNum = 1; overtoneNum <= maxOvertone; ++overtoneNum)
        {
            const auto overtoneMultiplier = (2 * overtoneNum) * m_skewEvens;
            const auto overtoneFreq = currentFreq * overtoneMultiplier;

            if (overtoneFreq >= maxFreq)
                break;

            const auto targetIndex = this->m_getFrequencyIndex(overtoneFreq);
            if (targetIndex >= this->m_frequencies.size())
                continue;

            const auto overtonePosition = static_cast<float>(overtoneNum - 1) / (maxOvertone - 1);
            auto overtonePower = this->calculateOvertonePower(power, m_evens, overtonePosition);
            overtonePower = this->applyPowerRandomness(overtonePower);

            this->triggerHarmonic(targetIndex, overtonePower, overtoneNum);
        }
    }
};

template <size_t NumElements>
class StretchedHarmonicGenerator : public HarmonicGeneratorBase<NumElements>
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

    void setStretched(float value) noexcept
    {
        m_stretched = value;
    }

    void generateHarmonics(size_t index, float power) override
    {
        if (m_stretched <= 0.0f)
            return;

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
                break;

            const auto targetIndex = this->m_getFrequencyIndex(overtoneFreq);
            if (targetIndex >= this->m_frequencies.size())
                continue;

            const auto overtonePosition = static_cast<float>(overtoneNum - 2) / (maxOvertone - 2);
            auto overtonePower = this->calculateOvertonePower(power, m_stretched, overtonePosition);
            overtonePower = this->applyPowerRandomness(overtonePower);

            this->triggerHarmonic(targetIndex, overtonePower, overtoneNum - 1);
        }
    }
};