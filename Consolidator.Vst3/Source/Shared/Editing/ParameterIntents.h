#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::shared::editing
{

struct ParameterIntents
{
    std::function<void(const std::string&, std::int64_t)> commit;
    std::function<bool(const std::string&, shared::protocol::ProtocolValue)> begin;
    std::function<void(shared::protocol::ProtocolValue)> change;
    std::function<void(shared::protocol::ProtocolValue)> end;
    std::function<void(const std::string&)> reset;
};

} // namespace consolidator::vst3::shared::editing
