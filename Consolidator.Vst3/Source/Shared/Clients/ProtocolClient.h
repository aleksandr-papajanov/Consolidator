#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::shared::clients
{

struct CommandResult
{
    std::uint64_t request;
    bool accepted;
    bool matched;
    std::string message;
};

class ProtocolClient final
{
public:
    using Completion = std::function<void(bool)>;
    using Transport = std::function<std::uint64_t(const std::string&, std::vector<shared::protocol::ProtocolValue>)>;

    explicit ProtocolClient(Transport transport);
    [[nodiscard]] std::uint64_t Request(const std::string& selector,
        std::vector<shared::protocol::ProtocolValue> body = {});
    void Send(const std::string& selector, std::vector<shared::protocol::ProtocolValue> body,
        Completion completion = {});
    [[nodiscard]] std::optional<CommandResult> Handle(const shared::protocol::ProtocolFrame& frame);
    void Clear();

private:
    Transport transport_;
    std::unordered_map<std::uint64_t, Completion> pending_;
};

} // namespace consolidator::vst3::shared::clients
