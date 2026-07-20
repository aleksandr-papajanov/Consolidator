#pragma once

#include "c74_min.h"
#include "Models/FilterDefinition.h"

#include <map>
#include <string>
#include <vector>

class ApproximatorOutputs final {
public:
    using Definitions = std::map<long, consolidator::models::FilterDefinition>;

    ApproximatorOutputs(
        c74::min::outlet<>& events,
        c74::min::outlet<>& status,
        c74::min::outlet<>& debug
    ) : events(events), status(status), debug(debug) {}

    void Ready(bool value) const {
        status.send("status", value ? "ready" : "idle");
    }

    void Loss(double value) const { debug.send("loss", value); }
    void Error(const char* value) const {
        status.send("status", "error", value);
        debug.send("error", value);
    }
    void FitStarted() const { status.send("status", "processing"); }

    void SendFitResult(
        const Definitions& definitions,
        const std::vector<double>& solverValues,
        long sessionId,
        long bankId,
        double loss
    ) const {
        c74::min::atoms output{
            "command", 1L, "approximator", sessionId, "fit.complete",
            sessionId, bankId, loss, static_cast<long>(definitions.size())
        };
        std::size_t offset = 0;
        for (const auto& [filterId, definition] : definitions) {
            output.push_back(filterId);
            output.push_back(0L);
            output.push_back(static_cast<long>(definition.parameters.size()));
            for (const auto& parameter : definition.parameters) {
                if (offset >= solverValues.size()) {
                    SendFitFailure(sessionId, "fit_result_size_mismatch");
                    return;
                }
                output.push_back(parameter.range.Denormalize(solverValues[offset++]));
            }
        }
        if (offset != solverValues.size()) {
            SendFitFailure(sessionId, "fit_result_size_mismatch");
            return;
        }
        events.send(output);
    }

    void SendFitFailure(long sessionId, const char* error) const {
        events.send("command", 1L, "approximator", sessionId, "fit.fail", sessionId, error);
    }

private:
    c74::min::outlet<>& events;
    c74::min::outlet<>& status;
    c74::min::outlet<>& debug;
};
