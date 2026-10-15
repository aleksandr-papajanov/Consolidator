#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Shared/Clients/ProtocolClient.h"

namespace consolidator::vst3::host::editor
{

struct EditorServices
{
    std::uint64_t instanceId;
    bool ready;
    shared::clients::ProtocolClient::Transport request;
    std::function<double()> sampleRate;
    std::function<bool()> windowFocused;
    std::function<void(const std::string&)> log;
    std::function<int()> cycleScale;
    std::function<void()> resizeEditor;
};

} // namespace consolidator::vst3::host::editor
