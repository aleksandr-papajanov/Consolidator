#pragma once

#include "c74_min.h"

#include "FilterContract.h"
#include "FilterRegistry.h"
#include "FilterSpec.h"
#include "MessageEnvelope.h"
#include "TypedMessages.h"

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

    void ready(bool available) const {
        status_out_.send("ready", available ? 1 : 0);
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

    void FitStarted() const {
        debug_out_.send("fit_started");
    }

    void FitFinished() const {
        debug_out_.send("fit_finished");
    }

    void FitResultSizeMismatch() const {
        debug_out_.send("error", "fit_result_size_mismatch");
    }

    void cleared() const {
        debug_out_.send("cleared");
    }

    void send_filter_commands(
        const FilterRegistry& registry,
        const std::vector<double>& solverValues,
        const long bankIndex
    ) const {
        std::size_t expected_count = 0;
        for (const auto& contract_opt : registry.all()) {
            if (contract_opt) {
                expected_count += contract_opt->parameters.size();
            }
        }
        if (solverValues.size() != expected_count) {
            FitResultSizeMismatch();
            return;
        }

        std::size_t value_offset = 0;
        for (const auto& contract_opt : registry.all()) {
            if (!contract_opt) {
                continue;
            }

            const auto& contract = *contract_opt;
            const auto count = contract.parameters.size();
            const std::vector<double> values(
                solverValues.begin() + value_offset,
                solverValues.begin() + value_offset + count);
            send_filter(contract, values, bankIndex);
            value_offset += count;
        }
    }

private:
    void send_filter(
        const FilterContract& contract,
        const std::vector<double>& values,
        const long bankIndex
    ) const {
        std::vector<double> absoluteValues;
        absoluteValues.reserve(values.size());
        for (std::size_t index = 0; index < values.size(); ++index) {
            absoluteValues.push_back(denormalize_parameter(
                contract.parameters[index].range, values[index]));
        }

        const consolidator::protocol::FilterApplyMessage command{
            contract.slot, absoluteValues, bankIndex };
        const auto message = command.to_envelope();
        commands_out_.send("message", message.transport_atom());
    }

    c74::min::outlet<>& commands_out_;
    c74::min::outlet<>& status_out_;
    c74::min::outlet<>& debug_out_;
};
