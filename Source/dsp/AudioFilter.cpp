#include "AudioFilter.h"
#include <cmath>
#include <array>

namespace wpdsp {

    void AudioFilter::prepare(double sampleRate) {
        m_sampleRate = sampleRate;
        biquad.reset();
        recalculateCoefficients();
    }

    void AudioFilter::reset() {
        biquad.reset();
    }

    double AudioFilter::processAudioSample(double sample) {
        if (m_coefficientsChanged)
        {
            recalculateCoefficients();
            m_coefficientsChanged = false;
        }
        // Original formula: return coeffArray[d0] * xn + coeffArray[c0] * biquad.processAudioSample(xn);
        return m_dry * sample + m_wet * biquad.processAudioSample(sample);
    }

    void AudioFilter::setAlgorithm(FilterAlgorithm newAlgorithm) {
        m_filterAlgorithm = newAlgorithm;
        m_coefficientsChanged = true;
    }

    void AudioFilter::setCutoff(double newCutoff) {
        m_freqCutoff = newCutoff;
        m_coefficientsChanged = true;
    }

    void AudioFilter::setQ(double newQ) {
        m_Q = newQ > 0 ? newQ : 0.707;
        m_coefficientsChanged = true;
    }

    void AudioFilter::setGainDb(double newGainDb) {
        m_gainDb = newGainDb;
        m_coefficientsChanged = true;
    }

    void AudioFilter::recalculateCoefficients() {
        if (m_sampleRate <= 0) return;

        std::array<double, numCoeffs> biquadCoeffs{};

        // Clear coeffs and set defaults (from original implementation)
        biquadCoeffs.fill(0.0);
        biquadCoeffs[a0] = 1.0;    // Default pass-through
    	m_wet = 1.0;
    	m_dry = 0.0;

        switch (m_filterAlgorithm) {
            case FilterAlgorithm::kImpInvLP1: {
                const double T = 1.0 / m_sampleRate;
                const double omega = 2.0 * M_PI * m_freqCutoff;
                const double eT = exp(-T * omega);

                biquadCoeffs[a0] = 1.0 - eT; // <--- normalized by 1-e^aT
                biquadCoeffs[a1] = 0.0;
                biquadCoeffs[a2] = 0.0;
                biquadCoeffs[b1] = -eT;
                biquadCoeffs[b2] = 0.0;
                break;
            }
        	case FilterAlgorithm::kImpInvLP2: {
				double alpha = 2.0 * M_PI * m_freqCutoff / m_sampleRate;
				double p_Re = -alpha / (2.0 * m_Q);
				double zeta = 1.0 / (2.0 * m_Q);
				double p_Im = alpha * pow((1.0 - (zeta * zeta)), 0.5);
				double c_Re = 0.0;
				double c_Im = alpha / (2.0 * pow((1.0 - (zeta*zeta)), 0.5));

				double eP_re = exp(p_Re);
				biquadCoeffs[a0] = c_Re;
				biquadCoeffs[a1] = -2.0*(c_Re*cos(p_Im) + c_Im*sin(p_Im))*exp(p_Re);
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = -2.0*eP_re*cos(p_Im);
				biquadCoeffs[b2] = eP_re*eP_re;
            	break;
			}
			// --- kMatchLP2A = TIGHT fit LPF vicanek algo
        	case FilterAlgorithm::kMatchLP2A: {
				// http://vicanek.de/articles/BiquadFits.pdf
				double theta_c = 2.0 * M_PI * m_freqCutoff / m_sampleRate;
				double q = 1.0 / (2.0 * m_Q);

				// --- impulse invariant
				double b_1 = 0.0;
				double b_2 = exp(-2.0 * q * theta_c);
				if (q <= 1.0)
				{
					b_1 = -2.0 * exp(-q * theta_c) * cos(pow((1.0 - q * q), 0.5) * theta_c);
				}
				else
				{
					b_1 = -2.0 * exp(-q * theta_c) * cosh(pow((q * q - 1.0), 0.5) * theta_c);
				}

				// --- TIGHT FIT --- //
				double B0 = (1.0 + b_1 + b_2) * (1.0 + b_1 + b_2);
				double B1 = (1.0 - b_1 + b_2) * (1.0 - b_1 + b_2);
				double B2 = -4.0 * b_2;

				double phi_0 = 1.0 - sin(theta_c / 2.0) * sin(theta_c / 2.0);
				double phi_1 = sin(theta_c / 2.0) * sin(theta_c / 2.0);
				double phi_2 = 4.0 * phi_0 * phi_1;

				double R1 = (B0 * phi_0 + B1 * phi_1 + B2 * phi_2) * (m_Q * m_Q);
				double A0 = B0;
				double A1 = (R1 - A0*phi_0) / phi_1;

				if (A0 < 0.0)
					A0 = 0.0;
				if (A1 < 0.0)
					A1 = 0.0;

				double a_0 = 0.5*(pow(A0, 0.5) + pow(A1, 0.5));
				double a_1 = pow(A0, 0.5) - a_0;
				double a_2 = 0.0;

				biquadCoeffs[a0] = a_0;
				biquadCoeffs[a1] = a_1;
				biquadCoeffs[a2] = a_2;
				biquadCoeffs[b1] = b_1;
				biquadCoeffs[b2] = b_2;
            	break;
			}
			// --- kMatchLP2B = LOOSE fit LPF vicanek algo
        	case FilterAlgorithm::kMatchLP2B: {
				// http://vicanek.de/articles/BiquadFits.pdf
				double theta_c = 2.0 * M_PI * m_freqCutoff / m_sampleRate;
				double q = 1.0 / (2.0 * m_Q);

				// --- impulse invariant
				double b_1 = 0.0;
				double b_2 = exp(-2.0 * q * theta_c);
				if (q <= 1.0)
				{
					b_1 = -2.0 * exp(-q * theta_c) * cos(pow((1.0 - q * q), 0.5) * theta_c);
				}
				else
				{
					b_1 = -2.0 * exp(-q * theta_c) * cosh(pow((q * q - 1.0), 0.5) * theta_c);
				}

				// --- LOOSE FIT --- //
				double f0 = theta_c / M_PI; // note f0 = fraction of pi, so that f0 = 1.0 = pi = Nyquist

				double r0 = 1.0 + b_1 + b_2;
				double denom = (1.0 - f0*f0)*(1.0 - f0*f0) + (f0*f0) / (m_Q*m_Q);
				denom = pow(denom, 0.5);
				double r1 = ((1.0 - b_1 + b_2)*f0*f0) / (denom);

				double a_0 = (r0 + r1) / 2.0;
				double a_1 = r0 - a_0;
				double a_2 = 0.0;

				biquadCoeffs[a0] = a_0;
				biquadCoeffs[a1] = a_1;
				biquadCoeffs[a2] = a_2;
				biquadCoeffs[b1] = b_1;
				biquadCoeffs[b2] = b_2;
            	break;
			}
			// --- kMatchBP2A = TIGHT fit BPF vicanek algo
        	case FilterAlgorithm::kMatchBP2A: {
				// http://vicanek.de/articles/BiquadFits.pdf
				double theta_c = 2.0 * M_PI * m_freqCutoff / m_sampleRate;
				double q = 1.0 / (2.0 * m_Q);

				// --- impulse invariant
				double b_1 = 0.0;
				double b_2 = exp(-2.0 * q * theta_c);
				if (q <= 1.0)
				{
					b_1 = -2.0 * exp(-q * theta_c) * cos(pow((1.0 - q * q), 0.5) * theta_c);
				}
				else
				{
					b_1 = -2.0 * exp(-q * theta_c) * cosh(pow((q * q - 1.0), 0.5) * theta_c);
				}

				// --- TIGHT FIT --- //
				double B0 = (1.0 + b_1 + b_2) * (1.0 + b_1 + b_2);
				double B1 = (1.0 - b_1 + b_2) * (1.0 - b_1 + b_2);
				double B2 = -4.0 * b_2;

				double phi_0 = 1.0 - sin(theta_c / 2.0) * sin(theta_c / 2.0);
				double phi_1 = sin(theta_c / 2.0) * sin(theta_c / 2.0);
				double phi_2 = 4.0 * phi_0 * phi_1;

				double R1 = B0 * phi_0 + B1 * phi_1 + B2 * phi_2;
				double R2 = -B0 + B1 + 4.0 * (phi_0 - phi_1) * B2;

				double A2 = (R1 - R2 * phi_1) / (4.0 * phi_1 * phi_1);
				double A1 = R2 + 4.0 * (phi_1 - phi_0) * A2;

				double a_1 = -0.5 * (pow(A1, 0.5));
				double a_0 = 0.5 *(pow((A2 + (a_1 * a_1)), 0.5) - a_1);
				double a_2 = -a_0 - a_1;

				biquadCoeffs[a0] = a_0;
				biquadCoeffs[a1] = a_1;
				biquadCoeffs[a2] = a_2;
				biquadCoeffs[b1] = b_1;
				biquadCoeffs[b2] = b_2;
            	break;
			}
			// --- kMatchBP2B = LOOSE fit BPF vicanek algo
        	case FilterAlgorithm::kMatchBP2B: {
				// http://vicanek.de/articles/BiquadFits.pdf
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double q = 1.0 / (2.0*m_Q);

				// --- impulse invariant
				double b_1 = 0.0;
				double b_2 = exp(-2.0*q*theta_c);
				if (q <= 1.0)
				{
					b_1 = -2.0*exp(-q*theta_c)*cos(pow((1.0 - q*q), 0.5)*theta_c);
				}
				else
				{
					b_1 = -2.0*exp(-q*theta_c)*cosh(pow((q*q - 1.0), 0.5)*theta_c);
				}

				// --- LOOSE FIT --- //
				double f0 = theta_c / M_PI; // note f0 = fraction of pi, so that f0 = 1.0 = pi = Nyquist

				double r0 = (1.0 + b_1 + b_2) / (M_PI*f0*m_Q);
				double denom = (1.0 - f0*f0)*(1.0 - f0*f0) + (f0*f0) / (m_Q*m_Q);
				denom = pow(denom, 0.5);

				double r1 = ((1.0 - b_1 + b_2)*(f0 / m_Q)) / (denom);

				double a_1 = -r1 / 2.0;
				double a_0 = (r0 - a_1) / 2.0;
				double a_2 = -a_0 - a_1;

				biquadCoeffs[a0] = a_0;
				biquadCoeffs[a1] = a_1;
				biquadCoeffs[a2] = a_2;
				biquadCoeffs[b1] = b_1;
				biquadCoeffs[b2] = b_2;
            	break;
			}
			case FilterAlgorithm::kLPF1P: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double gamma = 2.0 - cos(theta_c);

				double filter_b1 = pow((gamma*gamma - 1.0), 0.5) - gamma;
				double filter_a0 = 1.0 + filter_b1;

				// --- update coeffs
				biquadCoeffs[a0] = filter_a0;
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = filter_b1;
				biquadCoeffs[b2] = 0.0;
            	break;
			}
        	case FilterAlgorithm::kLPF1: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double gamma = cos(theta_c) / (1.0 + sin(theta_c));

				// --- update coeffs
				biquadCoeffs[a0] = (1.0 - gamma) / 2.0;
				biquadCoeffs[a1] = (1.0 - gamma) / 2.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = -gamma;
				biquadCoeffs[b2] = 0.0;
            	break;
			}
        	case FilterAlgorithm::kHPF1: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double gamma = cos(theta_c) / (1.0 + sin(theta_c));

				// --- update coeffs
				biquadCoeffs[a0] = (1.0 + gamma) / 2.0;
				biquadCoeffs[a1] = -(1.0 + gamma) / 2.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = -gamma;
				biquadCoeffs[b2] = 0.0;
            	break;
			}
        	case FilterAlgorithm::kLPF2: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double d = 1.0 / m_Q;
				double betaNumerator = 1.0 - ((d / 2.0)*(sin(theta_c)));
				double betaDenominator = 1.0 + ((d / 2.0)*(sin(theta_c)));

				double beta = 0.5*(betaNumerator / betaDenominator);
				double gamma = (0.5 + beta)*(cos(theta_c));
				double alpha = (0.5 + beta - gamma) / 2.0;

				// --- update coeffs
				biquadCoeffs[a0] = alpha;
				biquadCoeffs[a1] = 2.0*alpha;
				biquadCoeffs[a2] = alpha;
				biquadCoeffs[b1] = -2.0*gamma;
				biquadCoeffs[b2] = 2.0*beta;

				//double mag = getMagResponse(theta_c, biquadCoeffs[a0], biquadCoeffs[a1], biquadCoeffs[a2], biquadCoeffs[b1], biquadCoeffs[b2]);

            	break;
			}
        	case FilterAlgorithm::kHPF2: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double d = 1.0 / m_Q;

				double betaNumerator = 1.0 - ((d / 2.0)*(sin(theta_c)));
				double betaDenominator = 1.0 + ((d / 2.0)*(sin(theta_c)));

				double beta = 0.5*(betaNumerator / betaDenominator);
				double gamma = (0.5 + beta)*(cos(theta_c));
				double alpha = (0.5 + beta + gamma) / 2.0;

				// --- update coeffs
				biquadCoeffs[a0] = alpha;
				biquadCoeffs[a1] = -2.0*alpha;
				biquadCoeffs[a2] = alpha;
				biquadCoeffs[b1] = -2.0*gamma;
				biquadCoeffs[b2] = 2.0*beta;
            	break;
			}
        	case FilterAlgorithm::kBPF2: {
				// --- see book for formulae
				double K = tan(M_PI*m_freqCutoff / m_sampleRate);
				double delta = K*K*m_Q + K + m_Q;

				// --- update coeffs
				biquadCoeffs[a0] = K / delta;
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = -K / delta;
				biquadCoeffs[b1] = 2.0*m_Q*(K*K - 1) / delta;
				biquadCoeffs[b2] = (K*K*m_Q - K + m_Q) / delta;
            	break;
			}
        	case FilterAlgorithm::kBSF2: {
				// --- see book for formulae
				double K = tan(M_PI*m_freqCutoff / m_sampleRate);
				double delta = K*K*m_Q + K + m_Q;

				// --- update coeffs
				biquadCoeffs[a0] = m_Q*(1 + K*K) / delta;
				biquadCoeffs[a1] = 2.0*m_Q*(K*K - 1) / delta;
				biquadCoeffs[a2] = m_Q*(1 + K*K) / delta;
				biquadCoeffs[b1] = 2.0*m_Q*(K*K - 1) / delta;
				biquadCoeffs[b2] = (K*K*m_Q - K + m_Q) / delta;
            	break;
			}
        	case FilterAlgorithm::kButterLPF2: {
				// --- see book for formulae
				double theta_c = M_PI*m_freqCutoff / m_sampleRate;
				double C = 1.0 / tan(theta_c);

				// --- update coeffs
				biquadCoeffs[a0] = 1.0 / (1.0 + kSqrtTwo * C + C * C);
				biquadCoeffs[a1] = 2.0*biquadCoeffs[a0];
				biquadCoeffs[a2] = biquadCoeffs[a0];
				biquadCoeffs[b1] = 2.0*biquadCoeffs[a0] * (1.0 - C*C);
				biquadCoeffs[b2] = biquadCoeffs[a0] * (1.0 - kSqrtTwo*C + C*C);
            	break;
			}
        	case FilterAlgorithm::kButterHPF2: {
				// --- see book for formulae
				double theta_c = M_PI*m_freqCutoff / m_sampleRate;
				double C = tan(theta_c);

				// --- update coeffs
				biquadCoeffs[a0] = 1.0 / (1.0 + kSqrtTwo*C + C*C);
				biquadCoeffs[a1] = -2.0*biquadCoeffs[a0];
				biquadCoeffs[a2] = biquadCoeffs[a0];
				biquadCoeffs[b1] = 2.0*biquadCoeffs[a0] * (C*C - 1.0);
				biquadCoeffs[b2] = biquadCoeffs[a0] * (1.0 - kSqrtTwo*C + C*C);
            	break;
			}
        	case FilterAlgorithm::kButterBPF2: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double BW = m_freqCutoff / m_Q;
				double delta_c = M_PI*BW / m_sampleRate;
				if (delta_c >= 0.95*M_PI / 2.0) delta_c = 0.95*M_PI / 2.0;

				double C = 1.0 / tan(delta_c);
				double D = 2.0*cos(theta_c);

				// --- update coeffs
				biquadCoeffs[a0] = 1.0 / (1.0 + C);
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = -biquadCoeffs[a0];
				biquadCoeffs[b1] = -biquadCoeffs[a0] * (C*D);
				biquadCoeffs[b2] = biquadCoeffs[a0] * (C - 1.0);
				break;
			}
        	case FilterAlgorithm::kButterBSF2: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double BW = m_freqCutoff / m_Q;
				double delta_c = M_PI*BW / m_sampleRate;
				if (delta_c >= 0.95*M_PI / 2.0) delta_c = 0.95*M_PI / 2.0;

				double C = tan(delta_c);
				double D = 2.0*cos(theta_c);

				// --- update coeffs
				biquadCoeffs[a0] = 1.0 / (1.0 + C);
				biquadCoeffs[a1] = -biquadCoeffs[a0] * D;
				biquadCoeffs[a2] = biquadCoeffs[a0];
				biquadCoeffs[b1] = -biquadCoeffs[a0] * D;
				biquadCoeffs[b2] = biquadCoeffs[a0] * (1.0 - C);
            	break;
			}
        	case FilterAlgorithm::kMMALPF2:
        	case FilterAlgorithm::kMMALPF2B: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double resonance_dB = 0;

				if (m_Q > 0.707)
				{
					double peak = m_Q*m_Q / pow(m_Q*m_Q - 0.25, 0.5);
					resonance_dB = 20.0*log10(peak);
				}

				// --- intermediate vars
				double resonance = (cos(theta_c) + (sin(theta_c) * sqrt(pow(10.0, (resonance_dB / 10.0)) - 1))) / ((pow(10.0, (resonance_dB / 20.0)) * sin(theta_c)) + 1);
				double g = pow(10.0, (-resonance_dB / 40.0));

				// --- kMMALPF2B disables the GR with increase in Q
				if (m_filterAlgorithm == FilterAlgorithm::kMMALPF2B)
					g = 1.0;

				double filter_b1 = (-2.0) * resonance * cos(theta_c);
				double filter_b2 = resonance * resonance;
				double filter_a0 = g * (1 + filter_b1 + filter_b2);

				// --- update coeffs
				biquadCoeffs[a0] = filter_a0;
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = filter_b1;
				biquadCoeffs[b2] = filter_b2;
            	break;
			}
        	case FilterAlgorithm::kLowShelf: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double mu = pow(10.0, m_gainDb / 20.0);

				double beta = 4.0 / (1.0 + mu);
				double delta = beta*tan(theta_c / 2.0);
				double gamma = (1.0 - delta) / (1.0 + delta);

				// --- update coeffs
				biquadCoeffs[a0] = (1.0 - gamma) / 2.0;
				biquadCoeffs[a1] = (1.0 - gamma) / 2.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = -gamma;
				biquadCoeffs[b2] = 0.0;

				m_wet = mu - 1.0;
				m_dry = 1.0;
            	break;
			}
        	case FilterAlgorithm::kHiShelf: {
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double mu = pow(10.0, m_gainDb / 20.0);

				double beta = (1.0 + mu) / 4.0;
				double delta = beta*tan(theta_c / 2.0);
				double gamma = (1.0 - delta) / (1.0 + delta);

				biquadCoeffs[a0] = (1.0 + gamma) / 2.0;
				biquadCoeffs[a1] = -biquadCoeffs[a0];
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = -gamma;
				biquadCoeffs[b2] = 0.0;

				m_wet = mu - 1.0;
				m_dry = 1.0;
            	break;
			}
        	case FilterAlgorithm::kCQParaEQ: {
				// --- see book for formulae
				double K = tan(M_PI * m_freqCutoff / m_sampleRate);
				double Vo = pow(10.0, m_gainDb / 20.0);
				bool bBoost = m_gainDb >= 0;

				double d0 = 1.0 + (1.0 / m_Q) * K + K * K;
				double e0 = 1.0 + (1.0 / (Vo*m_Q)) * K + K * K;
				double alpha = 1.0 + (Vo / m_Q) * K + K * K;
				double beta = 2.0 * (K * K - 1.0);
				double gamma = 1.0 - (Vo / m_Q) * K + K*K;
				double delta = 1.0 - (1.0 / m_Q)*K + K*K;
				double eta = 1.0 - (1.0 / (Vo*m_Q))*K + K*K;

				// --- update coeffs
				biquadCoeffs[a0] = bBoost ? alpha / d0 : d0 / e0;
				biquadCoeffs[a1] = bBoost ? beta / d0 : beta / e0;
				biquadCoeffs[a2] = bBoost ? gamma / d0 : delta / e0;
				biquadCoeffs[b1] = bBoost ? beta / d0 : beta / e0;
				biquadCoeffs[b2] = bBoost ? delta / d0 : eta / e0;
            	break;
			}
        	case FilterAlgorithm::kNCQParaEQ: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double mu = pow(10.0, m_gainDb / 20.0);

				// --- clamp to 0.95 pi/2 (you can experiment with this)
				double tanArg = theta_c / (2.0 * m_Q);
				if (tanArg >= 0.95*M_PI / 2.0) tanArg = 0.95*M_PI / 2.0;

				// --- intermediate variables (you can condense this if you wish)
				double zeta = 4.0 / (1.0 + mu);
				double betaNumerator = 1.0 - zeta*tan(tanArg);
				double betaDenominator = 1.0 + zeta*tan(tanArg);

				double beta = 0.5*(betaNumerator / betaDenominator);
				double gamma = (0.5 + beta)*(cos(theta_c));
				double alpha = (0.5 - beta);

				// --- update coeffs
				biquadCoeffs[a0] = alpha;
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = -alpha;
				biquadCoeffs[b1] = -2.0*gamma;
				biquadCoeffs[b2] = 2.0*beta;

				m_wet = mu - 1.0;
				m_dry = 1.0;
            	break;
			}
        	case FilterAlgorithm::kLWRLPF2: {
				// --- see book for formulae
				double omega_c = M_PI*m_freqCutoff;
				double theta_c = M_PI*m_freqCutoff / m_sampleRate;

				double k = omega_c / tan(theta_c);
				double denominator = k*k + omega_c*omega_c + 2.0*k*omega_c;
				double b1_Num = -2.0*k*k + 2.0*omega_c*omega_c;
				double b2_Num = -2.0*k*omega_c + k*k + omega_c*omega_c;

				// --- update coeffs
				biquadCoeffs[a0] = omega_c*omega_c / denominator;
				biquadCoeffs[a1] = 2.0*omega_c*omega_c / denominator;
				biquadCoeffs[a2] = biquadCoeffs[a0];
				biquadCoeffs[b1] = b1_Num / denominator;
				biquadCoeffs[b2] = b2_Num / denominator;
            	break;
			}
        	case FilterAlgorithm::kLWRHPF2: {
				// --- see book for formulae
				double omega_c = M_PI*m_freqCutoff;
				double theta_c = M_PI*m_freqCutoff / m_sampleRate;

				double k = omega_c / tan(theta_c);
				double denominator = k*k + omega_c*omega_c + 2.0*k*omega_c;
				double b1_Num = -2.0*k*k + 2.0*omega_c*omega_c;
				double b2_Num = -2.0*k*omega_c + k*k + omega_c*omega_c;

				// --- update coeffs
				biquadCoeffs[a0] = k*k / denominator;
				biquadCoeffs[a1] = -2.0*k*k / denominator;
				biquadCoeffs[a2] = biquadCoeffs[a0];
				biquadCoeffs[b1] = b1_Num / denominator;
				biquadCoeffs[b2] = b2_Num / denominator;
            	break;
			}
        	case FilterAlgorithm::kAPF1: {
				// --- see book for formulae
				double alphaNumerator = tan((M_PI*m_freqCutoff) / m_sampleRate) - 1.0;
				double alphaDenominator = tan((M_PI*m_freqCutoff) / m_sampleRate) + 1.0;
				double alpha = alphaNumerator / alphaDenominator;

				// --- update coeffs
				biquadCoeffs[a0] = alpha;
				biquadCoeffs[a1] = 1.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = alpha;
				biquadCoeffs[b2] = 0.0;
            	break;
			}
        	case FilterAlgorithm::kAPF2: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double BW = m_freqCutoff / m_Q;
				double argTan = M_PI*BW / m_sampleRate;
				if (argTan >= 0.95*M_PI / 2.0) argTan = 0.95*M_PI / 2.0;

				double alphaNumerator = tan(argTan) - 1.0;
				double alphaDenominator = tan(argTan) + 1.0;
				double alpha = alphaNumerator / alphaDenominator;
				double beta = -cos(theta_c);

				// --- update coeffs
				biquadCoeffs[a0] = -alpha;
				biquadCoeffs[a1] = beta*(1.0 - alpha);
				biquadCoeffs[a2] = 1.0;
				biquadCoeffs[b1] = beta*(1.0 - alpha);
				biquadCoeffs[b2] = -alpha;
            	break;
			}
        	case FilterAlgorithm::kResonA: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double BW = m_freqCutoff / m_Q;
				double filter_b2 = exp(-2.0*M_PI*(BW / m_sampleRate));
				double filter_b1 = ((-4.0*filter_b2) / (1.0 + filter_b2))*cos(theta_c);
				double filter_a0 = (1.0 - filter_b2)*pow((1.0 - (filter_b1*filter_b1) / (4.0 * filter_b2)), 0.5);

				// --- update coeffs
				biquadCoeffs[a0] = filter_a0;
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = 0.0;
				biquadCoeffs[b1] = filter_b1;
				biquadCoeffs[b2] = filter_b2;
            	break;
			}
        	case FilterAlgorithm::kResonB: {
				// --- see book for formulae
				double theta_c = 2.0*M_PI*m_freqCutoff / m_sampleRate;
				double BW = m_freqCutoff / m_Q;
				double filter_b2 = exp(-2.0*M_PI*(BW / m_sampleRate));
				double filter_b1 = ((-4.0*filter_b2) / (1.0 + filter_b2))*cos(theta_c);
				double filter_a0 = 1.0 - pow(filter_b2, 0.5); // (1.0 - filter_b2)*pow((1.0 - (filter_b1*filter_b1) / (4.0 * filter_b2)), 0.5);

				// --- update coeffs
				biquadCoeffs[a0] = filter_a0;
				biquadCoeffs[a1] = 0.0;
				biquadCoeffs[a2] = -filter_a0;
				biquadCoeffs[b1] = filter_b1;
				biquadCoeffs[b2] = filter_b2;
            	break;
			}
            default: {
                // Default to a simple wire (pass-through) to prevent silence
                biquadCoeffs[a0] = 1.0; // biquad passes through
                m_wet = 1.0;
                m_dry = 0.0;
                break;
            }
        }

        biquad.setCoefficients(biquadCoeffs);
    }

}
