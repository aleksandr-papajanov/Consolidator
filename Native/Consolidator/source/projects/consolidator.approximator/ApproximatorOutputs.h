#pragma once

#include "c74_min.h"
#include "Models/FilterState.h"
#include "Models/ProcessorState.h"

#include <string>
#include <vector>

class ApproximatorOutputs final {
public:
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
    void SendFitResult(
        const std::vector<consolidator::models::FilterState>& filters,
        const consolidator::models::ProcessorState& processor,
        long sessionId,
        long bankId,
        double loss
    ) const {
        c74::min::atoms output{
            "command", 1L, "approximator", sessionId, "fit.complete",
            sessionId, bankId, loss, static_cast<long>(filters.size())
        };
        for (const auto& filter : filters) {
            output.push_back(filter.filterId);
            output.push_back(filter.bypass ? 1L : 0L);
            output.push_back(static_cast<long>(filter.values.size()));
            for (const auto value : filter.values) output.push_back(value);
        }
        output.push_back(processor.inputGain.gainDb);
        output.push_back(processor.compressor.bypass ? 1L : 0L);
        output.push_back(processor.compressor.attackMs);
        output.push_back(processor.compressor.releaseMs);
        output.push_back(processor.compressor.inputDb);
        output.push_back(processor.compressor.outputDb);
        output.push_back(processor.saturator.bypass ? 1L : 0L);
        output.push_back(processor.saturator.inputDb);
        output.push_back(processor.saturator.outputDb);
        output.push_back(processor.outputGain.gainDb);
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
