#pragma once

#include <optional>
#include <string>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::shared::clients
{

struct ParameterEntry
{
    std::string path;
    shared::protocol::ProtocolValue value;
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> effectiveMinimum;
    std::optional<double> effectiveMaximum;
};

} // namespace consolidator::vst3::shared::clients
