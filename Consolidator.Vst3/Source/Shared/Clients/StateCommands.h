#pragma once

#include <cstdint>
#include <string>

#include "Shared/Clients/ProtocolClient.h"
#include "Shared/Clients/TargetStateClient.h"

namespace consolidator::vst3::shared::clients
{

class StateCommands final
{
public:
    StateCommands(ProtocolClient& commands, const TargetStateClient& state);
    void WriteDiscrete(const std::string& path, std::int64_t value, bool groupScope,
        const std::string& transaction, ProtocolClient::Completion completion);
    void ResetValue(const std::string& path, bool groupScope, ProtocolClient::Completion completion);
    [[nodiscard]] static std::string DevicePath(const std::string& context);

private:
    ProtocolClient& commands_;
    const TargetStateClient& state_;
};

} // namespace consolidator::vst3::shared::clients
