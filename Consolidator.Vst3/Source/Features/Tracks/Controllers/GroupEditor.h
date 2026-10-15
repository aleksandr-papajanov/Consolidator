#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "Shared/Editing/DialGestureSession.h"

namespace consolidator::vst3::features::tracks::controllers
{

// Native port of the Max topology writer. Commands remain explicit-address
// writes to Managed, queued together just as in the existing Max controller.
class GroupEditor final
{
public:
    using Completion = shared::editing::DialGestureSession::Completion;
    using SendCommand = shared::editing::DialGestureSession::SendCommand;
    explicit GroupEditor(SendCommand send);
    void Write(std::vector<std::pair<std::uint64_t, int>> banks, shared::protocol::ProtocolValue group, Completion completion);

private:
    struct Operation
    {
        Completion completed;
        std::size_t remaining{};
        bool failed{};
    };
    SendCommand send_;
};

} // namespace consolidator::vst3::features::tracks::controllers
