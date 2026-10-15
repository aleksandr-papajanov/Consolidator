#include "Shared/Editing/DialGestureSession.h"

#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::editing
{

DialGestureSession::DialGestureSession(SendCommand send, std::function<void(bool)> finished)
    : send_(std::move(send)), finished_(std::move(finished))
{
}

DialGestureSession::~DialGestureSession()
{
    stopTimer();
}

bool DialGestureSession::Active() const { return session_.has_value(); }
bool DialGestureSession::Editing() const { return session_ && !session_->ending; }
const std::string& DialGestureSession::Path() const { return session_->path; }

void DialGestureSession::Begin(
    std::uint64_t transaction, std::string path, std::vector<shared::protocol::StateWriteEntry> values, std::string scope)
{
    if (session_)
    {
        throw std::logic_error("Only one control gesture may be active per editor.");
    }
    session_ = Session{ transaction, std::move(path), std::move(values), std::move(scope) };
    send_("begin_history", { std::to_string(transaction) }, [this, transaction](bool accepted)
    {
        if (!session_ || session_->transaction != transaction)
        {
            return;
        }
        if (!accepted)
        {
            Cancel();
            return;
        }
        session_->ready = true;
        if (session_->ending)
        {
            Finish();
        }
        else
        {
            Flush();
            startTimer(33);
        }
    });
}

void DialGestureSession::SetValues(std::vector<shared::protocol::StateWriteEntry> values)
{
    if (!session_ || session_->ending)
    {
        return;
    }
    session_->values = std::move(values);
    session_->dirty = true;
}

void DialGestureSession::End(std::vector<shared::protocol::StateWriteEntry> values)
{
    if (!session_ || session_->ending)
    {
        return;
    }
    session_->values = std::move(values);
    session_->dirty = true;
    session_->ending = true;
    stopTimer();
    if (session_->ready)
    {
        Finish();
    }
}

void DialGestureSession::timerCallback()
{
    Flush();
}

void DialGestureSession::Flush()
{
    if (!session_ || !session_->ready || !session_->dirty || session_->finalSent)
    {
        return;
    }
    session_->dirty = false;
    send_("write", shared::protocol::EncodeStateWrite(session_->values, session_->scope,
        std::to_string(session_->transaction)), {});
}

void DialGestureSession::Finish()
{
    if (!session_ || session_->finalSent)
    {
        return;
    }
    Flush();
    session_->finalSent = true;
    const auto transaction = session_->transaction;
    // The final coalesced write records history. This transaction-zero write
    // is a lossless acknowledgement after its preceding state notifications.
    send_("write", shared::protocol::EncodeStateWrite(session_->values, session_->scope, "0"),
        [this, transaction](bool accepted)
    {
        if (!session_ || session_->transaction != transaction)
        {
            return;
        }
        send_("end_history", { std::to_string(transaction) }, [this, transaction, accepted](bool ended)
        {
            if (!session_ || session_->transaction != transaction)
            {
                return;
            }
            session_.reset();
            finished_(accepted && ended);
        });
    });
}

void DialGestureSession::Cancel()
{
    if (!session_)
    {
        return;
    }
    const auto transaction = session_->transaction;
    stopTimer();
    session_.reset();
    // This also follows a queued begin when cancellation precedes its reply.
    send_("end_history", { std::to_string(transaction) }, {});
    finished_(false);
}

void DialGestureSession::Close()
{
    stopTimer();
    if (!session_)
    {
        return;
    }
    // Queue the latest position before detaching the editor. Managed's FIFO
    // orders these writes after begin even if its reply has not arrived yet.
    const auto transaction = std::to_string(session_->transaction);
    if (!session_->finalSent)
    {
        send_("write", shared::protocol::EncodeStateWrite(session_->values, session_->scope, transaction), {});
        send_("write", shared::protocol::EncodeStateWrite(session_->values, session_->scope, "0"), {});
    }
    send_("end_history", { transaction }, {});
    session_.reset();
}

} // namespace consolidator::vst3::shared::editing
