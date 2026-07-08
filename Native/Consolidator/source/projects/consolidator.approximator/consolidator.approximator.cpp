#include "c74_min.h"

#include "ApproximatorSupport.h"

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
            curve_store.set_target(args);

            ApproximatorOutputs outputs{ parameters_out, predicted_curve_out, debug_out };
            outputs.target_size(static_cast<int>(curve_store.curve().values.size()));

            return {};
        }
    };

    message<> fit_message{
        this,
        "fit",
        "Fit EQ parameters to current target curve",
        MIN_FUNCTION {
            ApproximatorOutputs outputs{ parameters_out, predicted_curve_out, debug_out };

            if (curve_store.empty()) {
                outputs.error("no_target_curve");
                return {};
            }

            try {
                const auto result = optimizer.fit(curve_store.curve(), outputs);
                outputs.loss(result.loss);
                outputs.send_parameters(result.params);
                outputs.send_curve(EqModel::buildCurve(curve_store.freqs(), result.params));
            }
            catch (const std::exception& e) {
                outputs.error(e.what());
            }

            return {};
        }
    };

    message<> clear_message{
        this,
        "clear",
        "Clear stored target curve",
        MIN_FUNCTION {
            curve_store.clear();
            ApproximatorOutputs outputs{ parameters_out, predicted_curve_out, debug_out };
            outputs.cleared();
            return {};
        }
    };

private:
    ApproximatorCurveStore curve_store;
    EqOptimizer optimizer;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
