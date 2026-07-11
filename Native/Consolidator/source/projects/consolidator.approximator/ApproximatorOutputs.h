#pragma once

#include "c74_min.h"

#include "EqParams.h"

class ApproximatorOutputs {
public:
    ApproximatorOutputs(
        c74::min::outlet<>& commands_out,
        c74::min::outlet<>& status_out,
        c74::min::outlet<>& debug_out
    ) :
        commands_out_(commands_out),
        status_out_(status_out),
        debug_out_(debug_out) {
    }

    void ready() const {
        status_out_.send("ready");
    }

    void capturing() const {
        status_out_.send("capturing");
    }

    void processing() const {
        status_out_.send("processing");
    }

    void done() const {
        status_out_.send("done");
    }

    void status_error() const {
        status_out_.send("error");
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

    void send_commands(const EqParams& p) const {
        commands_out_.send("gain", p.gainDb);
        commands_out_.send("tilt", p.tiltDb, p.tiltPivotHz);

        commands_out_.send("lowshelf", p.lowShelf.gainDb, p.lowShelf.freqHz, p.lowShelf.q);
        commands_out_.send("highshelf", p.highShelf.gainDb, p.highShelf.freqHz, p.highShelf.q);

        for (int i = 0; i < static_cast<int>(p.bells.size()); ++i) {
            const auto& b = p.bells[i];

            commands_out_.send(
                "bell",
                i,
                b.gainDb,
                b.freqHz,
                b.q
            );
        }
    }

private:
    c74::min::outlet<>& commands_out_;
    c74::min::outlet<>& status_out_;
    c74::min::outlet<>& debug_out_;
};
