#pragma once

#include "../IEqFilter.h"
#include "../../../Helpers/NumericHelper.h"
#include "../../../Settings/AnalysisOptions.h"
#include "../../../Settings/EqOptions.h"

#include <cmath>
#include <complex>
#include <numbers>

namespace consolidator::dsp {

class BiquadFilter : public IEqFilter {
protected:
    struct Coefficients {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a0 = 1.0;
        double a1 = 0.0;
        double a2 = 0.0;
    };

    struct Parameters {
        double frequencyHz;
        double q;
        double sampleRate;
    };

    class State {
    public:
        double ProcessSample(double input, const Coefficients& coefficients) {
            const double a0 = helpers::NumericHelper::SafeDenominator(coefficients.a0);
            const double output = coefficients.b0 / a0 * input + z1;
            z1 = coefficients.b1 / a0 * input - coefficients.a1 / a0 * output + z2;
            z2 = coefficients.b2 / a0 * input - coefficients.a2 / a0 * output;
            return output;
        }

        void Reset() {
            z1 = 0.0;
            z2 = 0.0;
        }

    private:
        double z1 = 0.0;
        double z2 = 0.0;
    };

    static Parameters Sanitize(double frequencyHz, double q, double sampleRate) {
        const double rate = helpers::NumericHelper::PositiveOr(sampleRate, 1.0);
        return {
            helpers::NumericHelper::ClampFinite(
                frequencyHz,
                settings::EqOptions::MinimumBiquadFrequencyHz,
                rate * settings::EqOptions::MaximumBiquadFrequencyRatio,
                settings::EqOptions::MinimumBiquadFrequencyHz),
            helpers::NumericHelper::PositiveOr(q, settings::EqOptions::MinimumBiquadQ),
            rate
        };
    }

    static double GetMagnitude(
        const Coefficients& coefficients,
        double frequencyHz,
        double sampleRate
    ) {
        return 20.0 * std::log10(helpers::NumericHelper::AtLeast(
            std::abs(Response(coefficients, frequencyHz, sampleRate)),
            settings::AnalysisOptions::MagnitudeNoiseFloor));
    }

    static double GetPhase(
        const Coefficients& coefficients,
        double frequencyHz,
        double sampleRate
    ) {
        return std::arg(Response(coefficients, frequencyHz, sampleRate));
    }

private:
    static std::complex<double> Response(
        const Coefficients& coefficients,
        double frequencyHz,
        double sampleRate
    ) {
        const auto parameters = Sanitize(frequencyHz, 1.0, sampleRate);
        const double a0 = helpers::NumericHelper::SafeDenominator(coefficients.a0);
        const double omega = 2.0 * std::numbers::pi * parameters.frequencyHz / parameters.sampleRate;
        const std::complex<double> z1 = std::exp(std::complex<double>{ 0.0, -omega });
        const std::complex<double> z2 = z1 * z1;
        const std::complex<double> numerator =
            coefficients.b0 / a0 + coefficients.b1 / a0 * z1 + coefficients.b2 / a0 * z2;
        const std::complex<double> denominator =
            1.0 + coefficients.a1 / a0 * z1 + coefficients.a2 / a0 * z2;
        return numerator / helpers::NumericHelper::SafeDenominator(std::abs(denominator));
    }

};

} // namespace consolidator::dsp
