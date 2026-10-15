#pragma once

#include <cstdint>
#include <optional>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::clients
{

struct GainTelemetry
{
    std::optional<double> inputLevel;
    std::optional<double> inputFast;
    std::optional<double> inputRms;
    std::optional<double> outputLevel;
    std::optional<double> outputFast;
    std::optional<double> outputRms;
    std::optional<double> compressorReduction;
    std::optional<double> limiterReduction;
};

// Accepts a bounded latest-only frame for the currently observed instance.
class GainTelemetryClient final
{
public:
    void Reset();
    [[nodiscard]] std::optional<GainTelemetry> Handle(const shared::protocol::ProtocolFrame& frame,
        std::uint64_t target);
    [[nodiscard]] const GainTelemetry& Latest() const;

private:
    std::int64_t revision_{};
    GainTelemetry latest_;
};

} // namespace consolidator::vst3::features::channelstrip::gainstaging::clients
