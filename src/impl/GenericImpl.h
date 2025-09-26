#pragma once

#include "EffectBase.h"
#include "Analysis/Spectrogram.h"
#include "Audio/AudioBuffer.h"

#include <cassert>
#include <cstdint>
#include <cmath>
#include <functional>

template <size_t BlockSize>
class GenericImpl final : public EffectBase
{
  public:
    GenericImpl(const float sampleRate)
        : EffectBase(sampleRate)
    {
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
        m_user1 = value;
    }
    void setUser10(const float value)
    {
        m_user10 = value;
    }
    void setUser2(const float value)
    {
        m_user2 = value;
    }
    void setUser3(const float value)
    {
        m_user3 = value;
    }
    void setUser5(const float value)
    {
        m_user5 = value;
    }
    void setUser4(const float value)
    {
        m_user4 = value;
    }
    void setUser6(const float value)
    {
        m_user6 = value;
    }
    void setUser7(const float value)
    {
        m_user7 = value;
    }
    void setUser8(const float value)
    {
        m_user8 = value;
    }
    void setUser9(const float value)
    {
        m_user9 = value;
    }
    void setUser11(const float value)
    {
        m_user11 = value;
    }
    void setUser12(const float value)
    {
        m_user12 = value;
    }
    void setUser13(const float value)
    {
        m_user13 = value;
    }
    void setUser14(const float value)
    {
        m_user14 = value;
    }
    void setUser15(const float value)
    {
        m_user15 = value;
    }

    void processBlock(const AbacDsp::AudioBuffer<2, BlockSize>& in, AbacDsp::AudioBuffer<2, BlockSize>& out)
    {
        for (size_t i = 0; i < BlockSize; ++i)
        {
            out(i, 0) = in(i, 0);
            out(i, 1) = in(i, 1);
        }
    }

  private:
    float m_vol{};
    float m_reverbLevel{};
    float m_user1{};
    float m_user10{};
    float m_user2{};
    float m_user3{};
    float m_user5{};
    float m_user4{};
    float m_user6{};
    float m_user7{};
    float m_user8{};
    float m_user9{};
    float m_user11{};
    float m_user12{};
    float m_user13{};
    float m_user14{};
    float m_user15{};
};