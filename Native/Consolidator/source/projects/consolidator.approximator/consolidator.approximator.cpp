#include "c74_min.h"

#include "ApproximatorCurveStore.h"
#include "ApproximatorOutputs.h"
#include "EqOptimizer.h"
#include "FilterContractDictionary.h"
#include "MessageEnvelope.h"
#include "MessageFactory.h"
#include "FilterRegistry.h"
#include "TypedMessages.h"

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
        "(message) commands: message <dictionary type=filter.define|approximator.clear|approximator.fit>"
    };

    outlet<> commands_out{
        this,
        "(message) commands: message <dictionary type=filter.update>"
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

    message<> loadbang{
        this,
        "loadbang",
        MIN_FUNCTION {
            set_ready(false, true);
            return {};
        }
    };

    message<> envelope_message{
        this,
        "message",
        "Receive a structured approximator control envelope",
        MIN_FUNCTION {
            if (inlet != 2 || args.size() != 1) {
                report_invalid_envelope(args);
                return {};
            }

            auto message = consolidator::protocol::MessageFactory::from_atom(args[0]);
            if (!message) {
                report_invalid_envelope(args);
                return {};
            }
            const auto result = consolidator::protocol::dispatch<
                consolidator::protocol::FilterDefineMessage,
                consolidator::protocol::ApproximatorClearMessage,
                consolidator::protocol::ApproximatorFitMessage>(*message, [this](const auto& command) {
                    handle_command(command);
                });
            if (result == consolidator::protocol::MessageDispatchResult::invalid) {
                report_invalid_typed_message(*message);
            }
            return {};
        }
    };

    ~ConsolidatorApproximator() override {
        if (fit_worker_.joinable()) {
            fit_worker_.join();
        }
    }

private:
    void report_invalid_envelope(const atoms& args) {
        atoms details{ "error", "invalid_message_envelope", "argument_count",
            static_cast<long>(args.size()), "arguments" };
        details.insert(details.end(), args.begin(), args.end());
        debug_out.send(details);
    }

    void report_invalid_typed_message(const consolidator::protocol::MessageEnvelope& message) {
        std::string type;
        std::string source;
        long target{};
        c74::min::dict payload;
        c74::min::atom contract;
        std::string contract_name;

        message.type(type);
        const bool has_target = message.target(target);
        const bool has_source = message.source(source);
        const bool has_payload = message.payload(payload);
        const bool has_contract_name = message.payload_symbol("contractName", contract_name);
        const bool has_contract_dictionary = message.payload_dictionary("contract", contract);

        atoms details{ "error", "invalid_message_envelope", "type", type,
            "has_target", has_target ? 1L : 0L,
            "target", target,
            "has_source", has_source ? 1L : 0L,
            "source", source,
            "has_payload", has_payload ? 1L : 0L,
            "has_contract_name", has_contract_name ? 1L : 0L,
            "contract_name", contract_name,
            "has_contract_dictionary", has_contract_dictionary ? 1L : 0L };
        debug_out.send(details);
    }

    void handle_command(const consolidator::protocol::FilterDefineMessage& command) {
        if (command.target < 0 || command.target >= static_cast<long>(FilterRegistry::max_filters)) {
            debug_out.send("error", "invalid_filter_definition");
            return;
        }
        FilterContract contract;
        const bool parsed = command.contractName.empty()
            ? parse_filter_contract_dictionary_for_slot(
                contract, command.contract, static_cast<int>(command.target))
            : [&]() {
                const dict configuration{ symbol(command.contractName.c_str()) };
                return parse_filter_contract_dictionary_for_slot(
                    contract,
                    atom{ static_cast<c74::max::t_object*>(configuration) },
                    static_cast<int>(command.target));
            }();
        if (!parsed) {
            debug_out.send("error", "invalid_filter_definition");
            return;
        }
        const bool was_empty = registry_.empty();
        registry_.define(contract);
        if (was_empty) update_ready();
    }

    void handle_command(const consolidator::protocol::ApproximatorClearMessage&) {
        curve_store.clear();
        has_recent_input_ = false;
        update_ready();
    }

    void handle_command(const consolidator::protocol::ApproximatorFitMessage&) { start_fit(); }

    void start_fit() {
        bool join_worker = false;
        ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
        if (registry_.empty()) {
            outputs.error("no_defined_filters");
            set_ready(false);
            return;
        }
        if (!has_recent_input_ || !curve_store.has_live_curve()) {
            outputs.error("no_difference_curve");
            set_ready(false);
            return;
        }
        if (!curve_store.has_current_eq_curve()) {
            outputs.error("no_current_eq_curve");
            set_ready(false);
            return;
        }
        if (!curve_store.has_compatible_curves()) {
            outputs.error("curve_size_mismatch");
            set_ready(false);
            return;
        }

        const auto curve = curve_store.combined_curve();
        const auto registry = registry_;
        {
            std::lock_guard<std::mutex> lock(fit_mutex_);
            if (fit_running_) {
                outputs.error("fit_in_progress");
                return;
            }
            join_worker = fit_worker_.joinable();
            fit_running_ = true;
            pending_error_.clear();
            pending_result_.reset();
        }
        if (join_worker) {
            fit_worker_.join();
        }
        set_ready(false);
        fit_worker_ = std::thread([this, curve, registry]() mutable {
            try {
                const auto result = optimizer.fit(curve, registry);
                std::lock_guard<std::mutex> lock(fit_mutex_);
                pending_result_ = result;
            }
            catch (const std::exception& error) {
                std::lock_guard<std::mutex> lock(fit_mutex_);
                pending_error_ = error.what();
            }
            fit_delivery.set();
        });
    }

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
