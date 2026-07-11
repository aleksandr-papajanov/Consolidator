#include "c74_min.h"

#include "EqProcessor.h"
#include "EqState.h"

using namespace c74::min;

class ConsolidatorEqualizer :
    public object<ConsolidatorEqualizer>,
    public sample_operator<2, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ device." };
    MIN_TAGS{ "audio, eq, filter" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> input_l{ this, "(signal) left input", "signal" };
    inlet<> input_r{ this, "(signal) right input", "signal" };
    inlet<> command_in{ this, "(anything) EQ commands" };

    outlet<> output_l{ this, "(signal) left output", "signal" };
    outlet<> output_r{ this, "(signal) right output", "signal" };

    outlet<> curve_out{ this, "(list) current EQ curve" };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                processor.set_sample_rate(static_cast<double>(args[0]));
                state_.set_sample_rate(static_cast<double>(args[0]));
            }

            processor.reset();
            publish_curve();
            return {};
        }
    };

    message<> gain_message{ this, "gain",
        MIN_FUNCTION {
            if (args.size() < 1) {
                return {};
            }

            state_.set_gain(static_cast<double>(args[0]));
            sync_processor();
            publish_curve();
            return {};
        }
    };

    message<> tilt_message{ this, "tilt",
        MIN_FUNCTION {
            if (args.size() < 2) {
                return {};
            }

            state_.set_tilt(static_cast<double>(args[0]), static_cast<double>(args[1]));
            sync_processor();
            publish_curve();
            return {};
        }
    };

    message<> lowshelf_message{ this, "lowshelf",
        MIN_FUNCTION {
            if (args.size() < 3) {
                return {};
            }

            state_.set_low_shelf(
                static_cast<double>(args[0]),
                static_cast<double>(args[1]),
                static_cast<double>(args[2]));
            sync_processor();
            publish_curve();
            return {};
        }
    };

    message<> highshelf_message{ this, "highshelf",
        MIN_FUNCTION {
            if (args.size() < 3) {
                return {};
            }

            state_.set_high_shelf(
                static_cast<double>(args[0]),
                static_cast<double>(args[1]),
                static_cast<double>(args[2]));
            sync_processor();
            publish_curve();
            return {};
        }
    };

    message<> bell_message{ this, "bell",
        MIN_FUNCTION {
            if (args.size() < 4) {
                return {};
            }

            const int index = static_cast<int>(args[0]);
            state_.set_bell(
                index,
                static_cast<double>(args[1]),
                static_cast<double>(args[2]),
                static_cast<double>(args[3]));
            sync_processor();
            publish_curve();
            return {};
        }
    };

    message<> reset_message{ this, "reset",
        MIN_FUNCTION {
            state_.reset();
            sync_processor();
            publish_curve();
            return {};
        }
    };

    message<> bang{ this, "bang",
        MIN_FUNCTION {
            publish_curve();
            return {};
        }
    };

    samples<2> operator()(sample in_l, sample in_r) {
        const EqParams params = state_.params();

        if (!params_equal(params, last_params_)) {
            processor.set_params(params);
            last_params_ = params;
        }

        const auto [out_l, out_r] = processor.process(in_l, in_r);
        return { out_l, out_r };
    }

private:
    static bool params_equal(const EqParams& a, const EqParams& b) {
        return a.gainDb == b.gainDb &&
            a.tiltDb == b.tiltDb &&
            a.tiltPivotHz == b.tiltPivotHz &&
            a.lowShelf.gainDb == b.lowShelf.gainDb &&
            a.lowShelf.freqHz == b.lowShelf.freqHz &&
            a.lowShelf.q == b.lowShelf.q &&
            a.highShelf.gainDb == b.highShelf.gainDb &&
            a.highShelf.freqHz == b.highShelf.freqHz &&
            a.highShelf.q == b.highShelf.q &&
            a.bells == b.bells;
    }

    void sync_processor() {
        const EqParams params = state_.params();
        if (!params_equal(params, last_params_)) {
            processor.set_params(params);
            last_params_ = params;
        }
    }

    void publish_curve() {
        c74::min::atoms out;
        const auto curve = state_.curve();
        out.reserve(curve.size());

        for (double value : curve) {
            out.push_back(value);
        }

        curve_out.send(out);
    }

    EqState state_;
    EqProcessor processor;
    EqParams last_params_{};
};

MIN_EXTERNAL_CUSTOM(ConsolidatorEqualizer, consolidator.equalizer);
