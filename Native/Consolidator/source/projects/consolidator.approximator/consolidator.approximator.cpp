#include "c74_min.h"

#include "ApproximatorSupport.h"
#include "FilterContractDictionary.h"
#include "FilterRegistry.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

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

    inlet<> current_eq_curve{
        this,
        "(list) current summed EQ curve in dB"
    };

    inlet<> commands{
        this,
        "(anything) commands: list, add_filter, dictionary, remove_filter, clear, fit"
    };

    outlet<> commands_out{
        this,
        "(anything) commands: filter, add_filter, remove_filter"
    };

    outlet<> status_out{
        this,
        "(anything) status: ready 0/1"
    };

    outlet<> debug_out{
        this,
        "(anything) diagnostics: error <code>, loss <value>"
    };

    queue<> fit_delivery{
        this,
        MIN_FUNCTION {
            deliver_fit_result();
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive the differential curve",
        MIN_FUNCTION {
            if (inlet == 0) {
                curve_store.set_target(args);
                has_recent_input_ = !args.empty();
                update_ready();
            }
            else if (inlet == 1) {
                curve_store.set_current_eq(args);
                update_ready();
            }

            return {};
        }
    };

    message<> clear_message{
        this,
        "clear",
        "Clear the current differential curve",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            curve_store.clear();
            has_recent_input_ = false;
            update_ready();
            return {};
        }
    };

    message<> loadbang{
        this,
        "loadbang",
        MIN_FUNCTION {
            set_ready(false, true);
            return {};
        }
    };

    message<> add_filter_message{
        this,
        "add_filter",
        "Register a filter contract",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            if (args.size() == 1 && dictionary_atom(args[0])) {
                FilterContract contract;
                if (!parse_filter_contract_dictionary(contract, args[0])) {
                    debug_out.send("error", "invalid_filter_configuration_dictionary");
                    return {};
                }

                const bool was_empty = registry_.empty();
                registry_.define(contract);
                if (was_empty) {
                    update_ready();
                }
                return {};
            }

            if (args.size() < 2) {
                return {};
            }

            const auto slot = static_cast<std::size_t>(static_cast<int>(args[0]));
            if (slot >= FilterRegistry::max_filters) {
                return {};
            }

            FilterContract contract;
            contract.slot = static_cast<int>(slot);
            if (!parse_definition_arguments(contract, args)) {
                return {};
            }

            const bool was_empty = registry_.empty();
            registry_.define(contract);
            if (was_empty) {
                update_ready();
            }
            return {};
        }
    };

    message<> dictionary_message{
        this,
        "dictionary",
        "Register a filter contract dictionary",
        MIN_FUNCTION {
            if (inlet != 2 || args.size() != 1 || !dictionary_atom(args[0])) {
                debug_out.send("error", "invalid_filter_configuration_dictionary");
                return {};
            }

            FilterContract contract;
            if (!parse_filter_contract_dictionary(contract, args[0])) {
                debug_out.send("error", "invalid_filter_configuration_dictionary");
                return {};
            }

            const bool was_empty = registry_.empty();
            registry_.define(contract);
            if (was_empty) {
                update_ready();
            }
            return {};
        }
    };

    message<> remove_filter_message{
        this,
        "remove_filter",
        "Remove one dynamically defined filter",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            if (args.size() != 1) {
                return {};
            }

            const size_t slot = static_cast<size_t>(args[0]);
            if (slot >= FilterRegistry::max_filters) {
                return {};
            }

            registry_.undefine(slot);
            update_ready();
            return {};
        }
    };

    message<> fit_message{
        this,
        "fit",
        "Fit EQ parameters to current target curve",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            bool join_worker = false;
            if (registry_.empty()) {
                ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                outputs.error("no_defined_filters");
                set_ready(false);
                return {};
            }

            if (!has_recent_input_ || !curve_store.has_live_curve()) {
                ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                outputs.error("no_difference_curve");
                set_ready(false);
                return {};
            }

            if (!curve_store.has_current_eq_curve()) {
                ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                outputs.error("no_current_eq_curve");
                set_ready(false);
                return {};
            }

            if (!curve_store.has_compatible_curves()) {
                ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                outputs.error("curve_size_mismatch");
                set_ready(false);
                return {};
            }

            const auto curve = curve_store.combined_curve();
            const auto registry = registry_;

            {
                std::lock_guard<std::mutex> lock(fit_mutex_);
                if (fit_running_) {
                    ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                    outputs.error("fit_in_progress");
                    return {};
                }

                if (fit_worker_.joinable()) {
                    join_worker = true;
                }

                fit_running_ = true;
                pending_error_.clear();
                pending_result_.reset();
            }

            if (join_worker) {
                fit_worker_.join();
            }

            ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
            set_ready(false);

            fit_worker_ = std::thread([this, curve, registry]() mutable {
                try {
                    const auto result = optimizer.fit(curve, registry);
                    {
                        std::lock_guard<std::mutex> lock(fit_mutex_);
                        pending_result_ = result;
                    }
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(fit_mutex_);
                    pending_error_ = e.what();
                }

                fit_delivery.set();
            });

            return {};
        }
    };

    ~ConsolidatorApproximator() override {
        if (fit_worker_.joinable()) {
            fit_worker_.join();
        }
    }

private:
    void deliver_fit_result() {
        std::optional<EqOptimizer::FitResult> result;
        std::string error;

        {
            std::lock_guard<std::mutex> lock(fit_mutex_);
            result = pending_result_;
            error = pending_error_;
            pending_result_.reset();
            pending_error_.clear();
        }

        if (fit_worker_.joinable()) {
            fit_worker_.join();
        }

        fit_running_ = false;

        ApproximatorOutputs outputs{ commands_out, status_out, debug_out };

        if (!error.empty()) {
            outputs.error(error.c_str());
            update_ready();
            return;
        }

        if (result) {
            outputs.loss(result->loss);
            outputs.send_filter_commands(registry_, result->normalized_values);
            update_ready();
        }
    }

    ApproximatorCurveStore curve_store;
    FilterRegistry registry_;
    EqOptimizer optimizer;
    std::mutex fit_mutex_;
    std::thread fit_worker_;
    std::atomic<bool> fit_running_ = false;
    std::optional<EqOptimizer::FitResult> pending_result_;
    std::string pending_error_;
    bool has_recent_input_ = false;
    bool ready_available_ = false;

    void update_ready() {
        set_ready(
            has_recent_input_ &&
            curve_store.has_compatible_curves() &&
            !registry_.empty() &&
            !fit_running_.load());
    }

    void set_ready(bool available, bool force = false) {
        if (!force && fit_running_.load()) {
            available = false;
        }

        if (!force && ready_available_ == available) {
            return;
        }

        ready_available_ = available;
        ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
        outputs.ready(available);
    }
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
