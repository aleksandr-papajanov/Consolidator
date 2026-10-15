#include "Shared/Clients/HistoryClient.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::clients
{

HistoryClient::HistoryClient(ProtocolClient& commands) : commands_(commands) { }

void HistoryClient::Undo(ProtocolClient::Completion completion)
{
    commands_.Send("jump_history", { UndoCursor() }, std::move(completion));
}

void HistoryClient::Redo(ProtocolClient::Completion completion)
{
    commands_.Send("jump_history", { RedoCursor() }, std::move(completion));
}

bool HistoryClient::Handle(const shared::protocol::ProtocolFrame& frame)
{
    if (frame.selector != "history_state")
    {
        return false;
    }
    if (frame.values.size() != 6)
    {
        throw std::invalid_argument("Invalid history state.");
    }
    const auto cursor = shared::protocol::Integer(frame.values[2]);
    const auto undo = shared::protocol::Boolean(frame.values[4]);
    const auto redo = shared::protocol::Boolean(frame.values[5]);
    cursor_ = cursor;
    canUndo_ = undo;
    canRedo_ = redo;
    return true;
}

std::int64_t HistoryClient::UndoCursor() const { return std::max<std::int64_t>(0, cursor_ - 1); }
std::int64_t HistoryClient::RedoCursor() const { return cursor_ + 1; }
bool HistoryClient::CanUndo() const { return canUndo_; }
bool HistoryClient::CanRedo() const { return canRedo_; }

} // namespace consolidator::vst3::shared::clients
