#pragma once

#include "Biquad.h"

namespace wpdsp {

    enum class FilterAlgorithm {
        kLPF1P, kLPF1, kHPF1, kLPF2, kHPF2, kBPF2, kBSF2, kButterLPF2, kButterHPF2, kButterBPF2,
        kButterBSF2, kMMALPF2, kMMALPF2B, kLowShelf, kHiShelf, kNCQParaEQ, kCQParaEQ, kLWRLPF2, kLWRHPF2,
        kAPF1, kAPF2, kResonA, kResonB, kMatchLP2A, kMatchLP2B, kMatchBP2A, kMatchBP2B,
        kImpInvLP1, kImpInvLP2
    };

    const double kSqrtTwo = pow(2.0, 0.5);

class AudioFilter {
public:
    AudioFilter() = default;
    ~AudioFilter() = default;

    void prepare (double sampleRate);
    void reset();
    double processAudioSample (double sample);

    // --- Parameter Setters ---
    void setAlgorithm (const FilterAlgorithm newAlgorithm) {
        m_filterAlgorithm = newAlgorithm;
        recalculateCoefficients();
    }

    void setCutoffFrequency (const double newFreq) {
        m_freqCutoff = newFreq;
        recalculateCoefficients();
    }

    void setQ (const double newQ) {
        if (newQ <= 0.0) {
            m_Q = 0.707;
        }
        recalculateCoefficients();
    }

    void setGain (const double newGainDb) {
        m_gainDb = newGainDb;
        recalculateCoefficients();
    }

private:
    void recalculateCoefficients();

    Biquad biquad;
    FilterAlgorithm m_filterAlgorithm = FilterAlgorithm::kLPF1;
    double m_sampleRate = 44100.0;
    double m_freqCutoff = 1000.0;
    double m_Q = 0.707;
    double m_gainDb = 0.0;

    double m_wet = 1.0;
    double m_dry = 0.0; // Dry mix
};

}
