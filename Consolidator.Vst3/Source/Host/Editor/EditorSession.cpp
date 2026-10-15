#include "Host/Editor/EditorSession.h"

#include <utility>

namespace consolidator::vst3::host::editor
{

EditorSession* EditorSession::activeSession_ = nullptr;

EditorSession::EditorSession(shared::clients::ProtocolClient& commands, std::function<bool()> windowFocused,
    EditorSessionEvents events)
    : commands_(commands), windowFocused_(std::move(windowFocused)), events_(std::move(events))
{
}

EditorSession::~EditorSession() { Close(); }
bool EditorSession::Active() const { return active_; }
bool EditorSession::Focused() const { return focused_; }

void EditorSession::Initialize()
{
    started_ = true;
    commands_.Send("initialize", {}, [this](bool accepted)
    {
        if (closed_)
        {
            return;
        }
        initialized_ = accepted;
        focused_ = false;
        if (!accepted)
        {
            events_.failed();
        }
    });
    startTimerHz(30);
}

void EditorSession::timerCallback()
{
    const bool focused = windowFocused_();
    const bool gained = focused && !focused_;
    const bool changed = focused != focused_;
    focused_ = focused;
    if (initialized_ && gained)
    {
        Activate();
    }
    if (changed)
    {
        events_.focusChanged();
    }
}

void EditorSession::Activate()
{
    if (closed_ || active_)
    {
        return;
    }
    if (activeSession_ != nullptr && activeSession_ != this)
    {
        activeSession_->active_ = false;
        activeSession_->focused_ = false;
        activeSession_->events_.deactivated();
    }
    activeSession_ = this;
    const auto generation = ++generation_;
    active_ = false;
    events_.connecting();
    commands_.Send("set_instance_active", { std::int64_t{ 1 } }, [this, generation](bool accepted)
    {
        if (closed_ || activeSession_ != this || generation != generation_)
        {
            return;
        }
        active_ = accepted;
        if (accepted)
        {
            events_.activated();
        }
        else
        {
            events_.failed();
        }
    });
}

void EditorSession::Fail()
{
    active_ = false;
    ++generation_;
    events_.focusChanged();
}

void EditorSession::Close()
{
    if (closed_)
    {
        return;
    }
    closed_ = true;
    stopTimer();
    active_ = false;
    if (activeSession_ == this)
    {
        activeSession_ = nullptr;
    }
    if (started_)
    {
        commands_.Send("set_instance_active", { std::int64_t{ 0 } });
    }
}

} // namespace consolidator::vst3::host::editor
