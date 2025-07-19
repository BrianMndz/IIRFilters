#include "Biquad.h"

#include <dispatch/data.h>

namespace wpdsp {

    void Biquad::reset(double sampleRate) {
        std::memset(m_state.data(), 0, sizeof(double) * numStates);
    }

    double Biquad::processAudioSample(double sample) {
        const auto& c = m_coefficients;
        auto& s = m_state;

        if (algorithm == BiquadAlgorithm::kDirect) {
            // --- 1)  form output y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2)
            const double yn = c[a0] * sample + c[a1] * s[x_z1] + c[a2] * s[x_z2] - c[b1] * s[y_z1] - c[b2] * s[y_z2];
            s[x_z2] = s[x_z1];
            s[x_z1] = sample;
            s[y_z2] = s[y_z1];
            s[y_z1] = s[yn];
            return yn;
        }
        if (algorithm == BiquadAlgorithm::kCanonical) {
            // --- 1)  form output y(n) = a0*w(n) + m_f_a1*stateArray[x_z1] + m_f_a2*stateArray[x_z2][x_z2];
            // --- w(n) = x(n) - b1*stateArray[x_z1] - b2*stateArray[x_z2]
            const double wn = sample - c[b1] * s[x_z1] - c[b2] * s[x_z2];
            const double yn = c[a0] * wn + c[a1] * s[x_z1] + c[a2] * s[x_z2];
            s[x_z2] = s[x_z1];
            s[x_z1] = wn;
            return yn;
        }

        if (algorithm == BiquadAlgorithm::kTransposeDirect) {
            // --- 1)  form output y(n) = a0*w(n) + stateArray[x_z1]
            // --- w(n) = x(n) + stateArray[y_z1]
            const double wn = sample + s[x_z1];
            const double yn = c[a0] * wn + s[y_z1];
            s[x_z1] = c[a1] * wn - c[b1] * yn + s[x_z2];
            s[x_z2] = c[a2] * wn - c[b2] * yn;
            return yn;
        }

        if (algorithm == BiquadAlgorithm::kTransposeCanonical) {
            // --- 1)  form output y(n) = a0*x(n) + stateArray[x_z1]
            const double yn = c[a0] * sample + s[x_z1];
            s[x_z1] = c[a1] * sample - c[b1] * yn + s[x_z2];
            s[x_z2] = c[a2] * sample - c[b2] * yn;
            return yn;
        }

        return sample; // Should not happen
    }

    void Biquad::setCoefficients(const double *coefficients) {
        std::memcpy(m_coefficients.data(), coefficients, sizeof(double) * numCoeffs);
    }

    void Biquad::setAlgorithm(BiquadAlgorithm newAlgo) {
        algorithm = newAlgo;
    }

}
