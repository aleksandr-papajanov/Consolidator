#pragma once

#include "ApproximatorOutputs.h"

#include "EqModel.h"
#include "EqParams.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

class EqOptimizer {
public:
    struct FitResult {
        EqParams params;
        double loss = 0.0;
    };

    FitResult fit(
        const TargetCurve& curve,
        ApproximatorOutputs& outputs
    ) const {
        if (curve.frequencies.size() != curve.values.size()) {
            throw std::runtime_error("freqs and targetDb size mismatch");
        }

        if (curve.frequencies.empty()) {
            throw std::runtime_error("empty curve");
        }

        EqParams best = initial_guess(curve.values);
        double best_loss = loss(curve, best);

        std::mt19937 rng{ 12345 };

        optimize_gain(best, best_loss, curve, rng);
        optimize_shape(best, best_loss, curve, rng);

        for (int bell_index = 0; bell_index < 4; ++bell_index) {
            optimize_bell(best, best_loss, curve, rng, bell_index, outputs);
        }

        for (int i = 0; i < 3000; ++i) {
            EqParams candidate = best;
            mutate(candidate, rng, i);

            const double candidate_loss = loss(curve, candidate);

            if (candidate_loss < best_loss) {
                best = candidate;
                best_loss = candidate_loss;
            }
        }

        outputs.final_loss(best_loss);
        return { best, best_loss };
    }

private:
    static EqParams initial_guess(const std::vector<double>& target_db) {
        EqParams p;

        double avg = 0.0;
        for (double v : target_db) {
            avg += v;
        }

        p.gainDb = avg / static_cast<double>(target_db.size());

        p.tiltDb = 0.0;
        p.tiltPivotHz = 1000.0;

        p.lowShelf.gainDb = 0.0;
        p.lowShelf.freqHz = 120.0;
        p.lowShelf.q = 0.707;

        p.highShelf.gainDb = 0.0;
        p.highShelf.freqHz = 8000.0;
        p.highShelf.q = 0.707;

        const double bell_freqs[4] = { 250.0, 800.0, 2500.0, 7000.0 };

        for (int i = 0; i < 4; ++i) {
            p.bells[i].gainDb = 0.0;
            p.bells[i].freqHz = bell_freqs[i];
            p.bells[i].q = 1.0;
        }

        return p;
    }

    static double loss(
        const TargetCurve& curve,
        const EqParams& params
    ) {
        const auto predicted = EqModel::buildCurve(curve.frequencies, params);

        double total = 0.0;

        for (size_t i = 0; i < predicted.size(); ++i) {
            const double err = predicted[i] - curve.values[i];
            total += perceptual_weight(curve.frequencies[i]) * err * err;
        }

        total += regularization(params);
        return total;
    }

    static double random_range(std::mt19937& rng, double min, double max) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }

    static void clamp_params(EqParams& p) {
        p.gainDb = std::clamp(p.gainDb, -18.0, 18.0);

        p.tiltDb = std::clamp(p.tiltDb, -18.0, 18.0);
        p.tiltPivotHz = std::clamp(p.tiltPivotHz, 200.0, 4000.0);

        p.lowShelf.gainDb = std::clamp(p.lowShelf.gainDb, -18.0, 18.0);
        p.lowShelf.freqHz = std::clamp(p.lowShelf.freqHz, 30.0, 800.0);
        p.lowShelf.q = std::clamp(p.lowShelf.q, 0.2, 2.0);

        p.highShelf.gainDb = std::clamp(p.highShelf.gainDb, -18.0, 18.0);
        p.highShelf.freqHz = std::clamp(p.highShelf.freqHz, 1500.0, 18000.0);
        p.highShelf.q = std::clamp(p.highShelf.q, 0.2, 2.0);

        for (auto& b : p.bells) {
            b.gainDb = std::clamp(b.gainDb, -18.0, 18.0);
            b.freqHz = std::clamp(b.freqHz, 40.0, 18000.0);
            b.q = std::clamp(b.q, 0.2, 8.0);
        }
    }

    static void optimize_gain(
        EqParams& best,
        double& best_loss,
        const TargetCurve& curve,
        std::mt19937& rng
    ) {
        for (int i = 0; i < 500; ++i) {
            EqParams candidate = best;
            candidate.gainDb += random_range(rng, -1.0, 1.0);

            clamp_params(candidate);

            const double l = loss(curve, candidate);
            if (l < best_loss) {
                best = candidate;
                best_loss = l;
            }
        }
    }

    static void optimize_shape(
        EqParams& best,
        double& best_loss,
        const TargetCurve& curve,
        std::mt19937& rng
    ) {
        for (int i = 0; i < 2000; ++i) {
            EqParams candidate = best;

            candidate.tiltDb += random_range(rng, -1.0, 1.0);

            candidate.lowShelf.gainDb += random_range(rng, -1.5, 1.5);
            candidate.lowShelf.freqHz *= std::pow(2.0, random_range(rng, -0.2, 0.2));
            candidate.lowShelf.q += random_range(rng, -0.15, 0.15);

            candidate.highShelf.gainDb += random_range(rng, -1.5, 1.5);
            candidate.highShelf.freqHz *= std::pow(2.0, random_range(rng, -0.2, 0.2));
            candidate.highShelf.q += random_range(rng, -0.15, 0.15);

            clamp_params(candidate);

            const double l = loss(curve, candidate);
            if (l < best_loss) {
                best = candidate;
                best_loss = l;
            }
        }
    }

    static void optimize_bell(
        EqParams& best,
        double& best_loss,
        const TargetCurve& curve,
        std::mt19937& rng,
        int bell_index,
        ApproximatorOutputs& outputs
    ) {
        for (int i = 0; i < 2500; ++i) {
            EqParams candidate = best;
            auto& b = candidate.bells[bell_index];

            b.gainDb += random_range(rng, -2.0, 2.0);
            b.freqHz *= std::pow(2.0, random_range(rng, -0.45, 0.45));
            b.q += random_range(rng, -0.5, 0.5);

            clamp_params(candidate);

            const double l = loss(curve, candidate);
            if (l < best_loss) {
                best = candidate;
                best_loss = l;
            }
        }

        outputs.bell_done(bell_index, best_loss);
    }

    static void mutate(
        EqParams& p,
        std::mt19937& rng,
        int iter
    ) {
        const double scale = iter < 3000 ? 1.0 : 0.25;

        p.gainDb += random_range(rng, -0.8, 0.8) * scale;
        p.tiltDb += random_range(rng, -0.8, 0.8) * scale;

        p.lowShelf.gainDb += random_range(rng, -1.0, 1.0) * scale;
        p.highShelf.gainDb += random_range(rng, -1.0, 1.0) * scale;

        p.lowShelf.freqHz *= std::pow(2.0, random_range(rng, -0.15, 0.15) * scale);
        p.highShelf.freqHz *= std::pow(2.0, random_range(rng, -0.15, 0.15) * scale);

        for (auto& b : p.bells) {
            b.gainDb += random_range(rng, -1.5, 1.5) * scale;

            const double octave_move = random_range(rng, -0.35, 0.35) * scale;
            b.freqHz *= std::pow(2.0, octave_move);

            b.q += random_range(rng, -0.35, 0.35) * scale;
        }

        clamp_params(p);
    }

    static double perceptual_weight(double f) {
        if (f < 40.0) {
            return 0.25;
        }

        if (f > 16000.0) {
            return 0.35;
        }

        return 1.0;
    }

    static double regularization(const EqParams& p) {
        double r = 0.0;

        r += 0.002 * p.gainDb * p.gainDb;
        r += 0.002 * p.tiltDb * p.tiltDb;

        r += 0.002 * p.lowShelf.gainDb * p.lowShelf.gainDb;
        r += 0.002 * p.highShelf.gainDb * p.highShelf.gainDb;

        for (const auto& b : p.bells) {
            r += 0.002 * b.gainDb * b.gainDb;
            r += 0.05 * std::max(0.0, b.q - 6.0) * std::max(0.0, b.q - 6.0);
        }

        return r;
    }
};
