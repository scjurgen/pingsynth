#pragma once

#include "EffectBase.h"
#include "Analysis/Spectrogram.h"
#include "Audio/AudioBuffer.h"
#include "PingSynth.h"

#include <cassert>
#include <cstdint>
#include <cmath>
#include <functional>

template <size_t BlockSize>
class PingSynthExplorerPedal final : public EffectBase
{
  public:
    static constexpr size_t NumChannels{2};
    PingSynthExplorerPedal(const float sampleRate)
        : EffectBase(sampleRate)
        , m_ping(sampleRate)
    {
    }

    void setReload(const bool value)
    {
        m_reload = value;
    }

    void setPreset(const float value)
    {
        m_preset = value;
    }

    void setVol(const float value)
    {
        m_vol = std::pow(10.f, value / 20.f);
    }

    void setReverbLevel(const float value)
    {
        m_reverbLevel = std::pow(10.f, value / 20.f);
    }

    void setUser1(const float value)
    {
        m_ping.setDecay(value * 0.01f);
    }

    void setUser2(const float value)
    {
        m_ping.setSpread(value * 0.01f);
    }

    void setUser3(const float value)
    {
        m_ping.setOddsOvertones(value * 0.01f);
    }

    void setUser4(const float value)
    {
        m_ping.setEvenOvertones(value * 0.01f);
    }

    void setUser5(const float value)
    {
        m_ping.setSkewOddOvertones(value * 0.01f);
    }

    void setUser6(const float value)
    {
        m_ping.setSkewEvenOvertones(value * 0.01f);
    }

    void setUser7(const float value)
    {
        m_ping.setStretchedOvertones(value * 0.01f);
    }

    void setUser8(const float value)
    {
        m_ping.setRandomSpread(value * 0.01f);
    }

    void setUser9(const float value)
    {
        m_ping.setRandomPower(value * 0.01f);
    }

    void setUser10(const float value)
    {
        m_ping.setDecaySkew(value * 0.01f);
    }
    void setUser11(const float value)
    {
        m_ping.setExcitationNoise(value * 0.01f);
    }

    void setUser12(const float value)
    {
        m_ping.setSparkleTime(value);
    }

    void setUser13(const float value)
    {
        m_ping.setSparkleRandom(value * 0.01f);
    }

    void setUser14(const float value)
    {
        m_ping.setMinOvertones(value);
    }

    void setUser15(const float value)
    {
        m_ping.setMaxOvertones(value);
    }

    [[maybe_unused]] void processMidi(const uint8_t* msg) override
    {
        switch (msg[0] & 0xF0)
        {
            case 0x90:
                m_ping.triggerVoice(msg[1], msg[2] / 127.f);
                break;
            case 0x80:
                m_ping.stopVoice(msg[1], msg[2] / 127.f);
                break;
            default:
                break;
        }
    }

    void processBlock(const AudioBuffer<NumChannels, BlockSize>& in, AudioBuffer<NumChannels, BlockSize>& out)
    {
        for (size_t i = 0; i < BlockSize; ++i)
        {
            out(i, 0) = in(i, 0);
            out(i, 1) = in(i, 1);
        }

        std::array<float, BlockSize> tmp{};
        m_ping.processBlock(tmp);
        for (size_t i = 0; i < BlockSize; ++i)
        {
            out(i, 0) += tmp[i] * m_vol;
            out(i, 1) += tmp[i] * m_vol;
        }
    }

  private:
    size_t m_reload{};
    float m_preset{};
    float m_vol{};
    float m_reverbLevel{};
    PingSynth<BlockSize> m_ping;
};
