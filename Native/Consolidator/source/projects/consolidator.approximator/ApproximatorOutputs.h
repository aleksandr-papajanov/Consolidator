#pragma once

#include "c74_min.h"

#include "FilterContract.h"
#include "FilterRegistry.h"
#include "FilterSpec.h"

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

    void cleared() const {
        debug_out_.send("cleared");
    }

    void send_filter_commands(
        const FilterRegistry& registry,
        const std::vector<double>& normalized_values
    ) const {
        std::size_t value_offset = 0;
        for (const auto& contract_opt : registry.all()) {
            if (!contract_opt) {
                continue;
            }

            const auto& contract = *contract_opt;
            const auto count = contract.parameters.size();
            if (value_offset + count > normalized_values.size()) {
                return;
            }

            const std::vector<double> values(
                normalized_values.begin() + value_offset,
                normalized_values.begin() + value_offset + count);
            send_filter(contract, contract_to_spec(contract, values));
            value_offset += count;
        }
    }

private:
    void send_filter(const FilterContract& contract, const FilterSpec& spec) const {
        commands_out_.send(make_definition_atoms(contract));
        commands_out_.send(make_filter_atoms(contract, spec));
    }

    c74::min::outlet<>& commands_out_;
    c74::min::outlet<>& status_out_;
    c74::min::outlet<>& debug_out_;
};
