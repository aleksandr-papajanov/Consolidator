#include "Shared/Editing/HistoryActionSession.h"

#include <string>
#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::editing
{

HistoryActionSession::HistoryActionSession(shared::clients::ProtocolClient& commands, std::uint64_t& nextTransaction)
    : commands_(commands), nextTransaction_(nextTransaction)
{
}

HistoryActionSession::~HistoryActionSession() { Close(); }

void HistoryActionSession::Close()
{
    if (closed_)
    {
        return;
    }
    closed_ = true;
    for (const auto& transaction : waitingBegin_)
    {
        commands_.Send("end_history", { transaction });
    }
    waitingBegin_.clear();
}

void HistoryActionSession::Run(std::function<void(const std::string&, shared::clients::ProtocolClient::Completion)> action,
    shared::clients::ProtocolClient::Completion completion)
{
    if (closed_)
    {
        throw std::logic_error("Cannot start an action after its editor session closed.");
    }
    const auto transaction = std::to_string(nextTransaction_++);
    waitingBegin_.insert(transaction);
    commands_.Send("begin_history", { transaction },
        [this, transaction, action = std::move(action), completion = std::move(completion)](bool accepted)
    {
        waitingBegin_.erase(transaction);
        if (closed_)
        {
            return;
        }
        if (!accepted)
        {
            completion(false);
            return;
        }
        try
        {
            action(transaction, completion);
        }
        catch (...)
        {
            commands_.Send("end_history", { transaction });
            throw;
        }
        // All action writes are queued before end, independently of their replies.
        commands_.Send("end_history", { transaction });
    });
}

} // namespace consolidator::vst3::shared::editing
