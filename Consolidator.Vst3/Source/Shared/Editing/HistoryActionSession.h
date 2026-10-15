#pragma once

#include <cstdint>
#include <functional>
#include <set>
#include <string>

#include "Shared/Clients/ProtocolClient.h"

namespace consolidator::vst3::shared::editing
{

class HistoryActionSession final
{
public:
    HistoryActionSession(shared::clients::ProtocolClient& commands, std::uint64_t& nextTransaction);
    ~HistoryActionSession();
    void Close();
    void Run(std::function<void(const std::string&, shared::clients::ProtocolClient::Completion)> action,
        shared::clients::ProtocolClient::Completion completion);

private:
    shared::clients::ProtocolClient& commands_;
    std::uint64_t& nextTransaction_;
    std::set<std::string> waitingBegin_;
    bool closed_{};
};

} // namespace consolidator::vst3::shared::editing
