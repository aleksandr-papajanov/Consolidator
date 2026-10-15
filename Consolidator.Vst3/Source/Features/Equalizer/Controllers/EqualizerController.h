#pragma once

#include <string>

#include "Shared/Clients/ProtocolClient.h"
#include "Shared/Clients/TargetStateClient.h"

namespace consolidator::vst3::features::equalizer::controllers
{

class EqualizerController final
{
public:
    explicit EqualizerController(shared::clients::ProtocolClient& commands);
    void Process(const std::string& selector, const shared::clients::UiTarget& target,
        bool groupScope, shared::clients::ProtocolClient::Completion completion);

private:
    shared::clients::ProtocolClient& commands_;
};

} // namespace consolidator::vst3::features::equalizer::controllers
