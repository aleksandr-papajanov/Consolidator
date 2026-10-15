#include "Shared/Clients/InputTelemetryClient.h"

#include <cmath>
#include <stdexcept>

namespace consolidator::vst3::shared::clients
{

void InputTelemetryClient::Reset() { revision_ = 0; }

std::optional<InputTelemetry> InputTelemetryClient::Handle(const shared::protocol::ProtocolFrame& frame,
    std::uint64_t target)
{
    const auto& values = frame.values;
    if (values.size() < 4)
    {
        throw std::invalid_argument("Truncated DSP telemetry frame.");
    }
    if (static_cast<std::uint64_t>(shared::protocol::Integer(values[1])) != target)
    {
        return {};
    }
    const auto revision = shared::protocol::Integer(values[2]);
    if (revision <= revision_)
    {
        return {};
    }
    if (shared::protocol::Integer(values[3]) != 2 || values.size() != 8 ||
        shared::protocol::Symbol(values[4]) != "input_gain.level" || shared::protocol::Symbol(values[6]) != "input_gain.level_fast")
    {
        throw std::invalid_argument("Invalid DSP telemetry frame.");
    }
    InputTelemetry result{ shared::protocol::Number(values[5]), shared::protocol::Number(values[7]) };
    if (!std::isfinite(result.slow) || !std::isfinite(result.fast))
    {
        throw std::invalid_argument("Non-finite Input telemetry.");
    }
    revision_ = revision;
    return result;
}

} // namespace consolidator::vst3::shared::clients
