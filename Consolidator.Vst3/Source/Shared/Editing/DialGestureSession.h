#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <JuceHeader.h>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::shared::editing
{

// Native port of Max DialGestureSession: wait for begin acknowledgement,
// retain the latest pending write batch/end, and never mutate application state here.
// Scalar dials and analyzer position/Q gestures share the same transaction lifecycle.
class DialGestureSession final : private juce::Timer
{
public:
    using Completion = std::function<void(bool)>;
    using SendCommand = std::function<void(const std::string&, std::vector<shared::protocol::ProtocolValue>, Completion)>;

    DialGestureSession(SendCommand send, std::function<void(bool)> finished);
    ~DialGestureSession() override;
    void Begin(std::uint64_t transaction, std::string path, std::vector<shared::protocol::StateWriteEntry> values, std::string scope);
    void SetValues(std::vector<shared::protocol::StateWriteEntry> values);
    void End(std::vector<shared::protocol::StateWriteEntry> values);
    void Cancel();
    void Close();
    [[nodiscard]] bool Active() const;
    [[nodiscard]] bool Editing() const;
    [[nodiscard]] const std::string& Path() const;

private:
    struct Session
    {
        std::uint64_t transaction;
        std::string path;
        std::vector<shared::protocol::StateWriteEntry> values;
        std::string scope;
        bool ready{};
        bool dirty{};
        bool ending{};
        bool finalSent{};
    };

    void timerCallback() override;
    void Flush();
    void Finish();

    SendCommand send_;
    std::function<void(bool)> finished_;
    std::optional<Session> session_;
};

} // namespace consolidator::vst3::shared::editing
