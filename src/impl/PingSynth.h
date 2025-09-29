#pragma once

#include <array>
#include <memory>
#include <random>
#include <numbers>
#include <functional>

#include "PingHarmonics.h"
#include "PingSpread.h"
#include "ResoGenerator.h"

template <size_t BlockSize>
class PingSynth
{
    static constexpr int minMidiNote{17};
    static constexpr int maxMidiNote{132};
    static constexpr int range{maxMidiNote - minMidiNote};
    static constexpr int stepsPerSemitone{66};

    static constexpr size_t NumElements{range * (stepsPerSemitone) + 1};

  public:
    explicit PingSynth(const float sampleRate)
        : m_sampleRate(sampleRate)
        , m_triggerCallback(
              [this](const size_t index, const float power, float order)
              {
                  size_t wait = 0;
                  if (m_sparkleRandom == 0.f || order == 0.f)
                  {
                      if (m_sparkleTimeBlocks < 0)
                      {
                          wait = static_cast<size_t>((1 - order) * -m_sparkleTimeBlocks);
                      }
                      else
                      {
                          wait = static_cast<size_t>(order * m_sparkleTimeBlocks);
                      }
                  }
                  else
                  {
                      std::uniform_real_distribution dist(0.f, 1.f);
                      const auto u = dist(m_randomGenerator);
                      if (m_sparkleTimeBlocks >= 0)
                      {
                          const auto interpolatedValue = (1.f - m_sparkleRandom) * order + m_sparkleRandom * u;
                          wait = static_cast<size_t>(interpolatedValue * m_sparkleTimeBlocks);
                      }
                      else
                      {
                          const auto interpolatedValue = (1.f - m_sparkleRandom) * (1 - order) + m_sparkleRandom * u;
                          wait = static_cast<size_t>(interpolatedValue * -m_sparkleTimeBlocks);
                      }
                  }
                  m_resoEngine.triggerNew(index, power, wait);
              })
        , m_getFrequencyIndexFunc(
              [this](const float targetFreq) -> size_t
              {
                  const auto baseFrequency = 440.0f * std::pow(2.0f, static_cast<float>(minMidiNote - 69) / 12.0f);
                  constexpr auto slotsPerOctave = static_cast<float>(stepsPerSemitone) * 12.0f;
                  const auto exactIndex = std::log2(targetFreq / baseFrequency) * slotsPerOctave;
                  const auto roundedIndex = static_cast<size_t>(std::round(exactIndex));
                  return std::clamp(roundedIndex, size_t{0}, m_frequencies.size() - 1);
              })
        , m_getRandomnessFunc(
              [this]() -> float
              {
                  std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
                  const auto u1 = uniform(m_randomGenerator);
                  const auto u2 = uniform(m_randomGenerator);
                  const auto gaussian =
                      std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * std::numbers::pi_v<float> * u2);
                  return std::clamp(gaussian * 0.3f, -1.0f, 1.0f);
              })
        , m_spreadCallback(
              [this](const size_t index, const float power)
              {
                  if (m_spreadGenerator)
                  {
                      m_spreadGenerator->generateSpreads(index, power);
                  }
              })
        , m_resoEngine(sampleRate, minMidiNote, stepsPerSemitone)
    {
        m_frequencies = m_resoEngine.getFrequencies();

        m_spreadGenerator = std::make_unique<PingSpread<NumElements, stepsPerSemitone>>(
            m_frequencies, m_getFrequencyIndexFunc, m_getRandomnessFunc, m_triggerCallback);

        m_oddGenerator = std::make_unique<OddHarmonicGenerator<NumElements>>(
            m_frequencies, m_getFrequencyIndexFunc, m_getRandomnessFunc, m_currentVelocity, m_randomPower,
            m_triggerCallback, m_spreadCallback);

        m_evenGenerator = std::make_unique<EvenHarmonicGenerator<NumElements>>(
            m_frequencies, m_getFrequencyIndexFunc, m_getRandomnessFunc, m_currentVelocity, m_randomPower,
            m_triggerCallback, m_spreadCallback);

        m_stretchedGenerator = std::make_unique<StretchedHarmonicGenerator<NumElements>>(
            m_frequencies, m_getFrequencyIndexFunc, m_getRandomnessFunc, m_currentVelocity, m_randomPower,
            m_triggerCallback, m_spreadCallback);
    }

    void setDecay(const float decay) noexcept
    {
        m_decay = decay;
        m_resoEngine.setDecay(decay);
    }

    void setDecaySkew(const float value) noexcept
    {
        m_resoEngine.setDecaySkew(value);
    }

    void setSpread(const float spread) noexcept
    {
        m_spreadGenerator->setSpread(spread);
    }

    void setOddsOvertones(const float value) noexcept
    {
        m_oddGenerator->setOdds(value);
    }

    void setEvenOvertones(const float value) noexcept
    {
        m_evenGenerator->setEvens(value);
    }

    void setStretchedOvertones(const float value) noexcept
    {
        m_stretchedGenerator->setStretched(value);
    }

    void setSkewOddOvertones(const float value)
    {
        m_oddGenerator->setSkewOdds(value);
    }

    void setSkewEvenOvertones(const float value)
    {
        m_evenGenerator->setSkewEvens(value);
    }

    void setRandomSpread(const float value)
    {
        m_spreadGenerator->setRandomSpread(value);
    }

    void setRandomPower(const float value)
    {
        m_randomPower = value;
        m_spreadGenerator->setRandomPower(value);
    }

    void setExcitationNoise(const float value)
    {
        m_resoEngine.setExcitationNoise(value);
    }

    void setSparkleTime(const float ms)
    {
        m_sparkleTimeBlocks = static_cast<int>(ms * 0.001f * m_sampleRate / BlockSize);
    }

    void setSparkleRandom(const float value)
    {
        m_sparkleRandom = value;
    }

    void setMinOvertones(const int overtones)
    {
        m_overtoneCount.first = overtones;
        m_oddGenerator->setMinMaxOvertone(m_overtoneCount);
        m_evenGenerator->setMinMaxOvertone(m_overtoneCount);
        m_stretchedGenerator->setMinMaxOvertone(m_overtoneCount);
    }

    void setMaxOvertones(const int overtones)
    {
        m_overtoneCount.second = overtones;
        m_oddGenerator->setMinMaxOvertone(m_overtoneCount);
        m_evenGenerator->setMinMaxOvertone(m_overtoneCount);
        m_stretchedGenerator->setMinMaxOvertone(m_overtoneCount);
    }

    void triggerSingleSlot(const size_t index, const float power) noexcept
    {
        m_triggerCallback(index, power, 0);
    }

    void triggerSlots(const size_t index, const float power) noexcept
    {
        triggerSingleSlot(index, power);
        m_spreadGenerator->generateSpreads(index, power);
        m_oddGenerator->generateHarmonics(index, power);
        m_evenGenerator->generateHarmonics(index, power);
        m_stretchedGenerator->generateHarmonics(index, power);
    }

    void triggerVoice(const size_t height, const float velocity) noexcept
    {
        std::cout << "\n" << height << "\t" << m_frequencies[height] << std::endl;
        if (height < minMidiNote || height > maxMidiNote)
        {
            return;
        }
        m_countVoices++;
        const auto relHeight = height - minMidiNote;
        const auto baseIdx = relHeight * stepsPerSemitone;
        const auto power = velocity * 20.f * (m_decay + 0.01f);
        m_currentVelocity = velocity;
        triggerSlots(baseIdx, power);
    }

    void stopVoice(const size_t height, const float /*velocity*/) noexcept
    {
        if (height < minMidiNote || height > maxMidiNote)
        {
            return;
        }
        m_countVoices--;
        if (m_countVoices == 0)
        {
        }
    }

    void setDamper(int value)
    {
        m_resoEngine.setDampMode(value > 63 ? false : true);
    }

    void processBlock(std::array<float, BlockSize>& out) noexcept
    {
        m_resoEngine.processBlock(out);
    }

  private:
    float getHumanRandomness() const noexcept
    {
        std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
        const auto u1 = uniform(m_randomGenerator);
        const auto u2 = uniform(m_randomGenerator);
        const auto gaussian = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * std::numbers::pi_v<float> * u2);
        return std::clamp(gaussian * 0.3f, -1.0f, 1.0f);
    }


    size_t getFrequencyIndex(const float targetFreq) const noexcept
    {
        const auto baseFrequency = 440.0f * std::pow(2.0f, static_cast<float>(minMidiNote - 69) / 12.0f);
        constexpr auto slotsPerOctave = static_cast<float>(stepsPerSemitone) * 12.0f;
        const auto exactIndex = std::log2(targetFreq / baseFrequency) * slotsPerOctave;
        const auto roundedIndex = static_cast<size_t>(std::round(exactIndex));
        return std::clamp(roundedIndex, size_t{0}, m_frequencies.size() - 1);
    }

    float m_sampleRate;
    float m_currentVelocity{1.0f};
    float m_randomPower{0.0f};
    size_t m_countVoices{0};
    int m_sparkleTimeBlocks{0};
    float m_sparkleRandom{0};
    float m_decay{0.f};

    std::array<float, NumElements> m_frequencies{};

    mutable std::mt19937 m_randomGenerator{std::random_device{}()};

    std::function<void(size_t, float, float)> m_triggerCallback;
    std::function<size_t(float)> m_getFrequencyIndexFunc;
    std::function<float()> m_getRandomnessFunc;
    std::function<void(size_t, float)> m_spreadCallback;
    std::pair<int, int> m_overtoneCount;
    std::unique_ptr<PingSpread<NumElements, stepsPerSemitone>> m_spreadGenerator;
    std::unique_ptr<OddHarmonicGenerator<NumElements>> m_oddGenerator;
    std::unique_ptr<EvenHarmonicGenerator<NumElements>> m_evenGenerator;
    std::unique_ptr<StretchedHarmonicGenerator<NumElements>> m_stretchedGenerator;
    ResoGenerator<BlockSize, NumElements> m_resoEngine;
};