#pragma once

#include <array>
#include <cmath>

namespace wpdsp {

    enum FilterCoeff { a0, a1, a2, b1, b2, c0, d0, numCoeffs };
    enum StateReg { x_z1, x_z2, y_z1, y_z2, numStates };
    enum class BiquadAlgorithm { kDirect, kCanonical, kTransposeDirect, kTransposeCanonical };

    class Biquad {
    public:
        Biquad() = default;
        ~Biquad() = default;

        void reset();
        [[nodiscard]] double processAudioSample (double sample);
        void setCoefficients (const std::array<double, numCoeffs>& coefficients);
        void setAlgorithm (BiquadAlgorithm newAlgo);

    private:
        std::array<double, numCoeffs> m_coefficients{};
        std::array<double, numStates> m_state{};
        BiquadAlgorithm m_biquadAlgorithm = BiquadAlgorithm::kDirect;
    };
}

