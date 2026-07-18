#pragma once

#include "c74_min.h"
#include "MaxMessageAdapter.h"
#include "Messaging/MessageFactory.h"
#include "Messaging/Messages/FilterApplyMessage.h"
#include "Models/FilterDefinition.h"
#include "Models/FilterState.h"

#include <map>
#include <vector>

class ApproximatorOutputs {
public:
    using Definitions = std::map<long, consolidator::models::FilterDefinition>;

    ApproximatorOutputs(
        c74::min::outlet<>& commands,
        c74::min::outlet<>& status,
        c74::min::outlet<>& debug
    ) : commands(commands), status(status), debug(debug) {}

    void Ready(bool value) const { status.send("ready", value ? 1 : 0); }
    void Loss(double value) const { debug.send("loss", value); }
    void Error(const char* value) const { debug.send("error", value); }
    void FitStarted() const { debug.send("fit_started"); }
    void FitFinished() const { debug.send("fit_finished"); }

    void SendFilterCommands(
        const Definitions& definitions,
        const std::vector<double>& solverValues,
        long bankIndex
    ) const {
        std::size_t offset = 0;
        for (const auto& [filterId, definition] : definitions) {
            consolidator::models::FilterState state;
            state.filterId = filterId;
            state.bankIndex = bankIndex;
            state.bypass = false;
            state.values.reserve(definition.parameters.size());
            for (const auto& parameter : definition.parameters) {
                if (offset >= solverValues.size()) {
                    Error("fit_result_size_mismatch");
                    return;
                }
                state.values.push_back(parameter.range.Denormalize(solverValues[offset++]));
            }
            const auto envelope = consolidator::messaging::MessageFactory::Create<
                consolidator::messaging::FilterApplyMessage>(
                    "approximator", "filter", std::move(state));
            commands.send("message", consolidator::maxadapter::MaxMessageAdapter::Serialize(envelope));
        }
        if (offset != solverValues.size()) Error("fit_result_size_mismatch");
    }

private:
    c74::min::outlet<>& commands;
    c74::min::outlet<>& status;
    c74::min::outlet<>& debug;
};
