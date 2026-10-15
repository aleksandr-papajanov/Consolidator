#pragma once

#include <cstdint>
#include <functional>

#include <JuceHeader.h>

#include "Shared/Clients/ProtocolClient.h"

namespace consolidator::vst3::host::editor
{

struct EditorSessionEvents
{
    std::function<void()> connecting;
    std::function<void()> activated;
    std::function<void()> deactivated;
    std::function<void()> failed;
    std::function<void()> focusChanged;
};

class EditorSession final : private juce::Timer
{
public:
    EditorSession(shared::clients::ProtocolClient& commands, std::function<bool()> windowFocused,
        EditorSessionEvents events);
    ~EditorSession() override;
    void Initialize();
    void Fail();
    void Close();
    [[nodiscard]] bool Active() const;
    [[nodiscard]] bool Focused() const;

private:
    void timerCallback() override;
    void Activate();

    shared::clients::ProtocolClient& commands_;
    std::function<bool()> windowFocused_;
    EditorSessionEvents events_;
    std::uint64_t generation_{};
    bool initialized_{};
    bool started_{};
    bool focused_{};
    bool active_{};
    bool closed_{};
    static EditorSession* activeSession_;
};

} // namespace consolidator::vst3::host::editor
