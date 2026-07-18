#pragma once

#include "c74_min.h"
#include "ComponentOutputs.h"
#include "Messaging/Messages/FilterSetManyMessage.h"
#include "Models/FilterDefinition.h"
#include "Models/FilterState.h"

#include <map>
#include <vector>

class ApproximatorOutputs {
public:
    using Definitions = std::map<long, consolidator::models::FilterDefinition>;

    explicit ApproximatorOutputs(consolidator::maxadapter::ComponentOutputs& outputs)
        : outputs(outputs) {}

    void Ready(bool value) const { outputs.Ready(value); }
    void Loss(double value) const { outputs.Debug("loss", value); }
    void Error(const char* value) const { outputs.Error(value); }
    void FitStarted() const { outputs.Debug("fit_started"); }
    void FitFinished() const { outputs.Debug("fit_finished"); }

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
            outputs.Send<consolidator::messaging::FilterSetManyMessage>(
                "approximator", "eq.storage", std::move(state));
        }
        if (offset != solverValues.size()) Error("fit_result_size_mismatch");
    }

private:
    consolidator::maxadapter::ComponentOutputs& outputs;
};
