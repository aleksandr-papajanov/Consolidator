#include "Features/ChannelStrip/GainStaging/Clients/GainTelemetryClient.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace consolidator::vst3::features::channelstrip::gainstaging::clients
{

void GainTelemetryClient::Reset()
{
    revision_ = 0;
    latest_ = {};
}

const GainTelemetry& GainTelemetryClient::Latest() const
{
    return latest_;
}

std::optional<GainTelemetry> GainTelemetryClient::Handle(const shared::protocol::ProtocolFrame& frame,
    std::uint64_t target)
{
    const auto& values = frame.values;
    if (values.size() < 4 || shared::protocol::Integer(values[0]) != 1)
    {
        throw std::invalid_argument("Invalid DSP telemetry header.");
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
    const auto count = shared::protocol::Integer(values[3]);
    if (count < 0 || count > 8 || values.size() != 4 + static_cast<std::size_t>(count) * 2)
    {
        throw std::invalid_argument("Invalid DSP telemetry size.");
    }
    GainTelemetry result;
    for (std::size_t index = 4; index < values.size(); index += 2)
    {
        const auto path = shared::protocol::Symbol(values[index]);
        const auto value = shared::protocol::Number(values[index + 1]);
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Non-finite DSP telemetry.");
        }
        std::optional<double>* slot{};
        if (path == "gain_staging.input_level")
        {
            slot = &result.inputLevel;
        }
        else if (path == "gain_staging.input_level_fast")
        {
            slot = &result.inputFast;
        }
        else if (path == "gain_staging.input_target")
        {
            slot = &result.inputRms;
        }
        else if (path == "gain_staging.output_level")
        {
            slot = &result.outputLevel;
        }
        else if (path == "gain_staging.output_level_fast")
        {
            slot = &result.outputFast;
        }
        else if (path == "gain_staging.output_target")
        {
            slot = &result.outputRms;
        }
        else if (path == "compression.reduction_db")
        {
            slot = &result.compressorReduction;
        }
        else if (path == "gain_staging.limiter_reduction_db")
        {
            slot = &result.limiterReduction;
        }
        else
        {
            throw std::invalid_argument("Unknown DSP telemetry path: " + std::string(path));
        }
        if (slot->has_value())
        {
            throw std::invalid_argument("Duplicate DSP telemetry path.");
        }
        *slot = value;
    }
    revision_ = revision;
    latest_ = result;
    return result;
}

} // namespace consolidator::vst3::features::channelstrip::gainstaging::clients
