#pragma once

#include <cstdint>
#include <optional>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::clients
{

struct InputTelemetry
{
    double slow;
    double fast;
};

class InputTelemetryClient final
{
public:
    void Reset();
    [[nodiscard]] std::optional<InputTelemetry> Handle(const shared::protocol::ProtocolFrame& frame,
        std::uint64_t target);

private:
    std::int64_t revision_{};
};

} // namespace consolidator::vst3::features::channelstrip::gainstaging::clients
