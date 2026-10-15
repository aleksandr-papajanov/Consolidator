#pragma once

#include <cstdint>

#include "Shared/Protocol/ProtocolFrame.h"
#include "Shared/Clients/ProtocolClient.h"

namespace consolidator::vst3::shared::clients
{

class HistoryClient final
{
public:
    explicit HistoryClient(ProtocolClient& commands);
    void Undo(ProtocolClient::Completion completion);
    void Redo(ProtocolClient::Completion completion);
    [[nodiscard]] bool Handle(const shared::protocol::ProtocolFrame& frame);
    [[nodiscard]] std::int64_t UndoCursor() const;
    [[nodiscard]] std::int64_t RedoCursor() const;
    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;

private:
    ProtocolClient& commands_;
    std::int64_t cursor_{};
    bool canUndo_{};
    bool canRedo_{};
};

} // namespace consolidator::vst3::shared::clients
