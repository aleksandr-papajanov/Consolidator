#include "Shared/Clients/ProtocolClient.h"

#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::clients
{

ProtocolClient::ProtocolClient(Transport transport) : transport_(std::move(transport))
{
}

std::uint64_t ProtocolClient::Request(const std::string& selector, std::vector<shared::protocol::ProtocolValue> body)
{
    return transport_(selector, std::move(body));
}

void ProtocolClient::Send(const std::string& selector, std::vector<shared::protocol::ProtocolValue> body,
    Completion completion)
{
    const auto request = Request(selector, std::move(body));
    if (request == 0)
    {
        if (completion)
        {
            completion(false);
        }
        return;
    }
    if (completion)
    {
        pending_.emplace(request, std::move(completion));
    }
}

std::optional<CommandResult> ProtocolClient::Handle(const shared::protocol::ProtocolFrame& frame)
{
    if (frame.selector != "initialized" && frame.selector != "action_done" && frame.selector != "error")
    {
        return {};
    }
    if (frame.values.empty() || shared::protocol::Integer(frame.values[0]) != 1)
    {
        throw std::invalid_argument("Unsupported Managed protocol frame.");
    }
    const auto& values = frame.values;
    if (values.size() < 3)
    {
        throw std::invalid_argument("Invalid protocol completion header.");
    }
    const bool error = frame.selector == "error";
    if ((frame.selector == "initialized" && values.size() != 5) ||
        (frame.selector == "action_done" && values.size() != 4) ||
        (error && values.size() != 4 && values.size() != 5))
    {
        throw std::invalid_argument("Invalid command completion.");
    }
    CommandResult result{ shared::protocol::WireId(values[2]),
        !error && (frame.selector == "initialized" || shared::protocol::Boolean(values[3])), false,
        error ? shared::protocol::Symbol(values[3]) : std::string{} };
    if (error && values.size() == 5)
    {
        // Protocol exceptions carry a code and message; rejected commands carry one message.
        result.message += ": " + shared::protocol::Symbol(values[4]);
    }
    const auto found = pending_.find(result.request);
    if (found != pending_.end())
    {
        result.matched = true;
        auto completion = std::move(found->second);
        pending_.erase(found);
        completion(result.accepted);
    }
    return result;
}

void ProtocolClient::Clear()
{
    pending_.clear();
}

} // namespace consolidator::vst3::shared::clients
