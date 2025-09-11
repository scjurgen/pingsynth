#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>

class BiquadBandPass
{
  public:
    BiquadBandPass()
    {
        computeCoefficients(1000.f);
    }

    explicit BiquadBandPass(const float sampleRate)
        : m_sampleRate(sampleRate)
    {
        computeCoefficients(1000.f);
    }
    void setSampleRate(const float sampleRate)
    {
        m_sampleRate = sampleRate;
    }

    float m_sampleRate{48000.f};

    void setByDecay(const float frequency, const float t)
    {
        constexpr auto k = 0.1447648273f; // 1.f / std::log(1000.f); //  1/6.9078f
        float Q = std::numbers::pi_v<float> * frequency * t * k;
        computeCoefficients(frequency, Q);
    }

    void computeCoefficients(const float frequency, const float Q = 1.f / std::numbers::sqrt2) noexcept
    {
        const auto Fc = frequency / m_sampleRate;
        const auto K = std::tan(std::numbers::pi_v<float> * Fc);
        const auto kqCl = K / std::max(Q, 0.01f);
        const auto kSquare = K * K;
        const auto norm = 1.f / (1 + kqCl + kSquare);
        m_b0 = kqCl * norm;
        m_a1 = 2 * (kSquare - 1) * norm;
        m_a2 = (1 - kqCl + kSquare) * norm;
    }

    float step(const float in) noexcept
    {
        const auto b0s = in * m_b0;
        const auto out = b0s + m_z[0];
        m_z[0] = m_z[1] - m_a1 * out;
        m_z[1] = -b0s - m_a2 * out;
        return out;
    }

    void process(const float* in, float* outBuffer, const size_t numSamples) noexcept
    {
        std::transform(in, in + numSamples, outBuffer, [this](const float v) { return step(v); });
    }

    void reset() noexcept
    {
        m_z[0] = 0.f;
        m_z[1] = 0.f;
    }

    float magnitude(const float hz, const float sampleRate = 48000.f) const noexcept
    {
        const auto phi = 4.f * std::pow(std::sin(2.f * std::numbers::pi_v<float> * hz / sampleRate / 2.f), 2.f);
        const auto db = 10.f * std::log10(std::pow((m_b0 + 0 + -m_b0), 2.f) +
                                          (m_b0 * -m_b0 * phi - (0 * (m_b0 + -m_b0) + 4.f * m_b0 * -m_b0)) * phi) -
                        10.f * std::log10(std::pow((1.f + m_a1 + m_a2), 2.f) +
                                          (m_a2 * phi - (m_a1 * (1.f + m_a2) + 4.f * m_a2)) * phi);
        return db;
    }

  private:
    float m_b0{0.f}, m_a1{0.f}, m_a2{0.f};
    std::array<float, 2> m_z{};
};
