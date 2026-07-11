#pragma once

#include <algorithm>
#include <cmath>

struct EqBiquadCoefficients {
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a0 = 1.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

struct EqBiquadState {
    double z1 = 0.0;
    double z2 = 0.0;

    double process(double input, const EqBiquadCoefficients& c) {
        const double b0 = c.b0 / c.a0;
        const double b1 = c.b1 / c.a0;
        const double b2 = c.b2 / c.a0;
        const double a1 = c.a1 / c.a0;
        const double a2 = c.a2 / c.a0;

        const double output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;

        return output;
    }

    void reset() {
        z1 = 0.0;
        z2 = 0.0;
    }
};

class EqBiquad {
public:
    static EqBiquadCoefficients peak(double freqHz, double q, double gainDb, double sampleRate) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * freqHz / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * q);

        EqBiquadCoefficients c;
        c.b0 = 1.0 + alpha * A;
        c.b1 = -2.0 * std::cos(w0);
        c.b2 = 1.0 - alpha * A;
        c.a0 = 1.0 + alpha / A;
        c.a1 = -2.0 * std::cos(w0);
        c.a2 = 1.0 - alpha / A;
        return c;
    }

    static EqBiquadCoefficients low_shelf(double freqHz, double q, double gainDb, double sampleRate) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * freqHz / sampleRate;
        const double cs = std::cos(w0);
        const double sn = std::sin(w0);
        const double alpha = sn / (2.0 * q);
        const double beta = 2.0 * std::sqrt(A) * alpha;

        EqBiquadCoefficients c;
        c.b0 = A * ((A + 1.0) - (A - 1.0) * cs + beta);
        c.b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cs);
        c.b2 = A * ((A + 1.0) - (A - 1.0) * cs - beta);
        c.a0 = (A + 1.0) + (A - 1.0) * cs + beta;
        c.a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cs);
        c.a2 = (A + 1.0) + (A - 1.0) * cs - beta;
        return c;
    }

    static EqBiquadCoefficients high_shelf(double freqHz, double q, double gainDb, double sampleRate) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * freqHz / sampleRate;
        const double cs = std::cos(w0);
        const double sn = std::sin(w0);
        const double alpha = sn / (2.0 * q);
        const double beta = 2.0 * std::sqrt(A) * alpha;

        EqBiquadCoefficients c;
        c.b0 = A * ((A + 1.0) + (A - 1.0) * cs + beta);
        c.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cs);
        c.b2 = A * ((A + 1.0) + (A - 1.0) * cs - beta);
        c.a0 = (A + 1.0) - (A - 1.0) * cs + beta;
        c.a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cs);
        c.a2 = (A + 1.0) - (A - 1.0) * cs - beta;
        return c;
    }

    static double response_db(
        double freqHz,
        double sampleRate,
        const EqBiquadCoefficients& c
    ) {
        const double b0 = c.b0 / c.a0;
        const double b1 = c.b1 / c.a0;
        const double b2 = c.b2 / c.a0;
        const double a1 = c.a1 / c.a0;
        const double a2 = c.a2 / c.a0;

        const double w = 2.0 * pi * freqHz / sampleRate;

        const double cos1 = std::cos(w);
        const double sin1 = std::sin(w);
        const double cos2 = std::cos(2.0 * w);
        const double sin2 = std::sin(2.0 * w);

        const double br = b0 + b1 * cos1 + b2 * cos2;
        const double bi = -b1 * sin1 - b2 * sin2;

        const double ar = 1.0 + a1 * cos1 + a2 * cos2;
        const double ai = -a1 * sin1 - a2 * sin2;

        const double bMag2 = br * br + bi * bi;
        const double aMag2 = ar * ar + ai * ai;
        const double mag = std::sqrt(bMag2 / std::max(1e-20, aMag2));

        return 20.0 * std::log10(std::max(1e-12, mag));
    }

private:
    static constexpr double pi = 3.1415926535897932384626433832795;
};
