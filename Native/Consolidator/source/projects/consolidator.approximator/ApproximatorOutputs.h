#pragma once

#include "c74_min.h"

#include <cmath>
#include <vector>

#include "EqParams.h"
#include "ApproximatorCurveStore.h"

class ApproximatorOutputs {
public:
    ApproximatorOutputs(
        c74::min::outlet<>& parameters_out,
        c74::min::outlet<>& predicted_curve_out,
        c74::min::outlet<>& debug_out
    ) :
        parameters_out_(parameters_out),
        predicted_curve_out_(predicted_curve_out),
        debug_out_(debug_out) {
    }

    void target_size(int size) const {
        debug_out_.send("target_size", size);
    }

    void loss(double value) const {
        debug_out_.send("loss", value);
    }

    void final_loss(double value) const {
        debug_out_.send("final_loss", value);
    }

    void bell_done(int bell_index, double value) const {
        debug_out_.send("bell_done", bell_index, value);
    }

    void error(const char* message) const {
        debug_out_.send("error", message);
    }

    void cleared() const {
        debug_out_.send("cleared");
    }

    void send_parameters(const EqParams& p) const {
        parameters_out_.send("gain", p.gainDb);
        parameters_out_.send("tilt", p.tiltDb, p.tiltPivotHz);

        parameters_out_.send("lowshelf", p.lowShelf.gainDb, p.lowShelf.freqHz, p.lowShelf.q);
        parameters_out_.send("highshelf", p.highShelf.gainDb, p.highShelf.freqHz, p.highShelf.q);

        for (int i = 0; i < static_cast<int>(p.bells.size()); ++i) {
            const auto& b = p.bells[i];

            parameters_out_.send(
                "bell",
                i,
                b.gainDb,
                b.freqHz,
                b.q,
                std::abs(b.gainDb) < 0.05 ? "inactive" : "active"
            );
        }
    }

    void send_curve(const std::vector<double>& curve) const {
        c74::min::atoms out;
        out.reserve(curve.size());

        for (double v : curve) {
            out.push_back(v);
        }

        predicted_curve_out_.send(out);
    }

    void send_curve(const TargetCurve& curve) const {
        send_curve(curve.values);
    }

private:
    c74::min::outlet<>& parameters_out_;
    c74::min::outlet<>& predicted_curve_out_;
    c74::min::outlet<>& debug_out_;
};
