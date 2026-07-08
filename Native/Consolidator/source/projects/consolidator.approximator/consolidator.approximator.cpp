#include "c74_min.h"
 
#include <vector>
#include <cmath>
#include <stdexcept>

#include "EqParams.h"
#include "EqModel.h"

using namespace c74::min;

class ConsolidatorApproximator : public object<ConsolidatorApproximator> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ curve approximator." };
    MIN_TAGS{ "audio, eq, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> input_curve{
        this,
        "(list) target difference curve in dB"
    };

    inlet<> commands{
        this,
        "(anything) commands: fit, clear"
    };

    outlet<> parameters_out{
        this,
        "(list) fitted EQ parameters"
    };

    outlet<> predicted_curve_out{
        this,
        "(list) predicted EQ curve"
    };

    outlet<> debug_out{
        this,
        "(anything) debug info"
    };

    message<> list{
        this,
        "list",
        "Receive target difference curve",
        MIN_FUNCTION {
            targetDb.clear();
            targetDb.reserve(args.size());

            for (const auto& a : args)
                targetDb.push_back(static_cast<double>(a));

            freqs = makeLogFreqs(targetDb.size(), 20.0, 20000.0);

            debug_out.send("target_size", static_cast<int>(targetDb.size()));
            return {};
        }
    };

    message<> fit_message{
        this,
        "fit",
        "Fit EQ parameters to current target curve",
        MIN_FUNCTION {
            if (targetDb.empty()) {
                debug_out.send("error", "no_target_curve");
                return {};
            }

            try {
                const EqParams params = fit(freqs, targetDb);

                const double finalLoss = loss(freqs, targetDb, params);
                debug_out.send("loss", finalLoss);

                const auto predicted = EqModel::buildCurve(freqs, params);

                outputParams(params);
                outputCurve(predicted);
            }
            catch (const std::exception& e) {
                debug_out.send("error", e.what());
            }

            return {};
        }
    };

    message<> clear_message{
        this,
        "clear",
        "Clear stored target curve",
        MIN_FUNCTION {
            targetDb.clear();
            freqs.clear();
            debug_out.send("cleared");
            return {};
        }
    };

    EqParams fit(
        const std::vector<double>& freqs,
        const std::vector<double>& targetDb
    ) {
        if (freqs.size() != targetDb.size())
            throw std::runtime_error("freqs and targetDb size mismatch");

        if (freqs.empty())
            throw std::runtime_error("empty curve");

        EqParams best = initialGuess(targetDb);
        double bestLoss = loss(freqs, targetDb, best);

        std::mt19937 rng{ 12345 };

        optimizeGain(best, bestLoss, freqs, targetDb, rng);
        optimizeShape(best, bestLoss, freqs, targetDb, rng);

        for (int bellIndex = 0; bellIndex < 4; ++bellIndex)
            optimizeBell(best, bestLoss, freqs, targetDb, rng, bellIndex);

        for (int i = 0; i < 3000; ++i) {
            EqParams candidate = best;
            mutate(candidate, rng, i);

            const double candidateLoss = loss(freqs, targetDb, candidate);

            if (candidateLoss < bestLoss) {
                best = candidate;
                bestLoss = candidateLoss;
            }
        }

        debug_out.send("final_loss", bestLoss);
        return best;
    }

private:
    std::vector<double> freqs;
    std::vector<double> targetDb;

    static std::vector<double> makeLogFreqs(
        size_t count,
        double minHz,
        double maxHz
    ) {
        std::vector<double> result;
        result.reserve(count);

        const double minLog = std::log(minHz);
        const double maxLog = std::log(maxHz);

        for (size_t i = 0; i < count; ++i) {
            const double t = count <= 1
                ? 0.0
                : static_cast<double>(i) / static_cast<double>(count - 1);

            result.push_back(std::exp(minLog + t * (maxLog - minLog)));
        }

        return result;
    }

    void outputCurve(const std::vector<double>& curve) {
        atoms out;
        out.reserve(curve.size());

        for (double v : curve)
            out.push_back(v);

        predicted_curve_out.send(out);
    }

    void outputParams(const EqParams& p) {
        parameters_out.send("gain", p.gainDb);
        parameters_out.send("tilt", p.tiltDb, p.tiltPivotHz);

        parameters_out.send("lowshelf", p.lowShelf.gainDb, p.lowShelf.freqHz, p.lowShelf.q);
        parameters_out.send("highshelf", p.highShelf.gainDb, p.highShelf.freqHz, p.highShelf.q);

        for (int i = 0; i < static_cast<int>(p.bells.size()); ++i) {
            const auto& b = p.bells[i];

            parameters_out.send(
                "bell",
                i,
                b.gainDb,
                b.freqHz,
                b.q,
                std::abs(b.gainDb) < 0.05 ? "inactive" : "active"
            );
        }
    }

    void optimizeGain(
        EqParams& best,
        double& bestLoss,
        const std::vector<double>& freqs,
        const std::vector<double>& targetDb,
        std::mt19937& rng
    ) {
        for (int i = 0; i < 500; ++i) {
            EqParams candidate = best;
            candidate.gainDb += randomRange(rng, -1.0, 1.0);

            clampParams(candidate);

            const double l = loss(freqs, targetDb, candidate);
            if (l < bestLoss) {
                best = candidate;
                bestLoss = l;
            }
        }
    }

    void optimizeShape(
        EqParams& best,
        double& bestLoss,
        const std::vector<double>& freqs,
        const std::vector<double>& targetDb,
        std::mt19937& rng
    ) {
        for (int i = 0; i < 2000; ++i) {
            EqParams candidate = best;

            candidate.tiltDb += randomRange(rng, -1.0, 1.0);

            candidate.lowShelf.gainDb += randomRange(rng, -1.5, 1.5);
            candidate.lowShelf.freqHz *= std::pow(2.0, randomRange(rng, -0.2, 0.2));
            candidate.lowShelf.q += randomRange(rng, -0.15, 0.15);

            candidate.highShelf.gainDb += randomRange(rng, -1.5, 1.5);
            candidate.highShelf.freqHz *= std::pow(2.0, randomRange(rng, -0.2, 0.2));
            candidate.highShelf.q += randomRange(rng, -0.15, 0.15);

            clampParams(candidate);

            const double l = loss(freqs, targetDb, candidate);
            if (l < bestLoss) {
                best = candidate;
                bestLoss = l;
            }
        }
    }

    void optimizeBell(
        EqParams& best,
        double& bestLoss,
        const std::vector<double>& freqs,
        const std::vector<double>& targetDb,
        std::mt19937& rng,
        int bellIndex
    ) {
        for (int i = 0; i < 2500; ++i) {
            EqParams candidate = best;
            auto& b = candidate.bells[bellIndex];

            b.gainDb += randomRange(rng, -2.0, 2.0);
            b.freqHz *= std::pow(2.0, randomRange(rng, -0.45, 0.45));
            b.q += randomRange(rng, -0.5, 0.5);

            clampParams(candidate);

            const double l = loss(freqs, targetDb, candidate);
            if (l < bestLoss) {
                best = candidate;
                bestLoss = l;
            }
        }

        debug_out.send("bell_done", bellIndex, bestLoss);
    }

    static EqParams initialGuess(const std::vector<double>& targetDb) {
        EqParams p;

        double avg = 0.0;
        for (double v : targetDb)
            avg += v;

        p.gainDb = avg / static_cast<double>(targetDb.size());

        p.tiltDb = 0.0;
        p.tiltPivotHz = 1000.0;

        p.lowShelf.gainDb = 0.0;
        p.lowShelf.freqHz = 120.0;
        p.lowShelf.q = 0.707;

        p.highShelf.gainDb = 0.0;
        p.highShelf.freqHz = 8000.0;
        p.highShelf.q = 0.707;

        const double bellFreqs[4] = { 250.0, 800.0, 2500.0, 7000.0 };

        for (int i = 0; i < 4; ++i) {
            p.bells[i].gainDb = 0.0;
            p.bells[i].freqHz = bellFreqs[i];
            p.bells[i].q = 1.0;
        }

        return p;
    }

    static double loss(
        const std::vector<double>& freqs,
        const std::vector<double>& targetDb,
        const EqParams& params
    ) {
        const auto predicted = EqModel::buildCurve(freqs, params);

        double total = 0.0;

        for (size_t i = 0; i < predicted.size(); ++i) {
            const double err = predicted[i] - targetDb[i];
            total += perceptualWeight(freqs[i]) * err * err;
        }

        total += regularization(params);

        return total;
    }

    static double randomRange(
        std::mt19937& rng,
        double min,
        double max
    ) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }


    static void clampParams(EqParams& p) {
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

    static void mutate(
        EqParams& p,
        std::mt19937& rng,
        int iter
    ) {
        const double scale = iter < 3000 ? 1.0 : 0.25;

        p.gainDb += randomRange(rng, -0.8, 0.8) * scale;
        p.tiltDb += randomRange(rng, -0.8, 0.8) * scale;

        p.lowShelf.gainDb += randomRange(rng, -1.0, 1.0) * scale;
        p.highShelf.gainDb += randomRange(rng, -1.0, 1.0) * scale;

        p.lowShelf.freqHz *= std::pow(2.0, randomRange(rng, -0.15, 0.15) * scale);
        p.highShelf.freqHz *= std::pow(2.0, randomRange(rng, -0.15, 0.15) * scale);

        for (auto& b : p.bells) {
            b.gainDb += randomRange(rng, -1.5, 1.5) * scale;

            const double octaveMove = randomRange(rng, -0.35, 0.35) * scale;
            b.freqHz *= std::pow(2.0, octaveMove);

            b.q += randomRange(rng, -0.35, 0.35) * scale;
        }

        clampParams(p);
    }

    static double perceptualWeight(double f) {
        // Пока простая версия:
        // меньше доверяем самому низу и самому верху
        if (f < 40.0) return 0.25;
        if (f > 16000.0) return 0.35;
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

    static void fillBounds(
        std::vector<double>& lower,
        std::vector<double>& upper
    ) {
        int i = 0;

        // global gain
        lower[i] = -18.0; upper[i] = 18.0; i++;

        // tiltDb, logFreq pivot
        lower[i] = -18.0; upper[i] = 18.0; i++;
        lower[i] = std::log(200.0); upper[i] = std::log(4000.0); i++;

        // low shelf: gain, logFreq, Q
        lower[i] = -18.0; upper[i] = 18.0; i++;
        lower[i] = std::log(30.0); upper[i] = std::log(800.0); i++;
        lower[i] = 0.2; upper[i] = 2.0; i++;

        // high shelf: gain, logFreq, Q
        lower[i] = -18.0; upper[i] = 18.0; i++;
        lower[i] = std::log(1500.0); upper[i] = std::log(18000.0); i++;
        lower[i] = 0.2; upper[i] = 2.0; i++;

        // 4 bells: gain, logFreq, Q
        for (int b = 0; b < 4; ++b) {
            lower[i] = -18.0; upper[i] = 18.0; i++;
            lower[i] = std::log(40.0); upper[i] = std::log(18000.0); i++;
            lower[i] = 0.2; upper[i] = 8.0; i++;
        }
    }
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
