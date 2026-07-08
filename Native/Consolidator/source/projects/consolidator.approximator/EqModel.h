#pragma once

#include "EqParams.h"

#include <algorithm>
#include <cmath>
#include <vector>

class EqModel {
public:
    static std::vector<double> buildCurve(
        const std::vector<double>& freqs,
        const EqParams& p,
        double sampleRate = 48000.0
    ) {
        std::vector<double> result;
        result.reserve(freqs.size());

        for (double f : freqs) {
            double db = 0.0;

            db += p.gainDb;
            db += tiltResponse(f, p.tiltDb, p.tiltPivotHz);
            db += lowShelfResponse(f, p.lowShelf, sampleRate);
            db += highShelfResponse(f, p.highShelf, sampleRate);

            for (const auto& bell : p.bells)
                db += bellResponse(f, bell, sampleRate);

            result.push_back(db);
        }

        return result;
    }

private:
    static constexpr double pi = 3.14159265358979323846;

    static double bellResponse(
        double f,
        const BellParams& p,
        double sampleRate
    ) {
        return biquadPeakResponseDb(f, p.freqHz, p.q, p.gainDb, sampleRate);
    }

    static double lowShelfResponse(
        double f,
        const ShelfParams& p,
        double sampleRate
    ) {
        return biquadLowShelfResponseDb(f, p.freqHz, p.q, p.gainDb, sampleRate);
    }

    static double highShelfResponse(
        double f,
        const ShelfParams& p,
        double sampleRate
    ) {
        return biquadHighShelfResponseDb(f, p.freqHz, p.q, p.gainDb, sampleRate);
    }

    static double tiltResponse(
        double f,
        double tiltDb,
        double pivotHz
    ) {
        const double oct = std::log2(f / pivotHz);
        return tiltDb * oct / 8.0;
    }

    static double biquadPeakResponseDb(
        double f,
        double freqHz,
        double q,
        double gainDb,
        double sampleRate
    ) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * freqHz / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * q);

        double b0 = 1.0 + alpha * A;
        double b1 = -2.0 * std::cos(w0);
        double b2 = 1.0 - alpha * A;
        double a0 = 1.0 + alpha / A;
        double a1 = -2.0 * std::cos(w0);
        double a2 = 1.0 - alpha / A;

        return biquadMagnitudeDb(f, sampleRate, b0, b1, b2, a0, a1, a2);
    }

    static double biquadLowShelfResponseDb(
        double f,
        double freqHz,
        double q,
        double gainDb,
        double sampleRate
    ) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * freqHz / sampleRate;
        const double cs = std::cos(w0);
        const double sn = std::sin(w0);
        const double alpha = sn / (2.0 * q);
        const double beta = 2.0 * std::sqrt(A) * alpha;

        double b0 = A * ((A + 1.0) - (A - 1.0) * cs + beta);
        double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cs);
        double b2 = A * ((A + 1.0) - (A - 1.0) * cs - beta);
        double a0 = (A + 1.0) + (A - 1.0) * cs + beta;
        double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cs);
        double a2 = (A + 1.0) + (A - 1.0) * cs - beta;

        return biquadMagnitudeDb(f, sampleRate, b0, b1, b2, a0, a1, a2);
    }

    static double biquadHighShelfResponseDb(
        double f,
        double freqHz,
        double q,
        double gainDb,
        double sampleRate
    ) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * freqHz / sampleRate;
        const double cs = std::cos(w0);
        const double sn = std::sin(w0);
        const double alpha = sn / (2.0 * q);
        const double beta = 2.0 * std::sqrt(A) * alpha;

        double b0 = A * ((A + 1.0) + (A - 1.0) * cs + beta);
        double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cs);
        double b2 = A * ((A + 1.0) + (A - 1.0) * cs - beta);
        double a0 = (A + 1.0) - (A - 1.0) * cs + beta;
        double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cs);
        double a2 = (A + 1.0) - (A - 1.0) * cs - beta;

        return biquadMagnitudeDb(f, sampleRate, b0, b1, b2, a0, a1, a2);
    }

    static double biquadMagnitudeDb(
        double f,
        double sampleRate,
        double b0,
        double b1,
        double b2,
        double a0,
        double a1,
        double a2
    ) {
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        const double w = 2.0 * pi * f / sampleRate;

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
};