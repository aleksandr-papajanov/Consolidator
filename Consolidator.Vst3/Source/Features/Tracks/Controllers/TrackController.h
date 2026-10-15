#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Shared/Clients/ProtocolClient.h"
#include "Shared/Clients/RegistryClient.h"
#include "Shared/Clients/TargetStateClient.h"
#include "Features/Tracks/Bindings/TrackIntent.h"

namespace consolidator::vst3::features::tracks::controllers
{

class TrackController final
{
public:
    using Completion = shared::clients::ProtocolClient::Completion;
    using Banks = std::vector<std::pair<std::uint64_t, int>>;
    TrackController(shared::clients::ProtocolClient& commands, const shared::clients::RegistryClient& registry);
    void Execute(const features::tracks::bindings::TrackIntent& intent,
        const shared::clients::UiTarget& target, bool groupScope,
        Completion completion);
    void Group(Banks banks, Completion completion);
    void Ungroup(const shared::clients::UiTarget& target, Completion completion);
    void Clear(Completion completion);

private:
    shared::clients::ProtocolClient& commands_;
    const shared::clients::RegistryClient& registry_;
};

} // namespace consolidator::vst3::features::tracks::controllers
