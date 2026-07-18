#pragma once

#include "Application/ComponentStatus.h"
#include "MaxDictionarySerializer.h"
#include "Messaging/MessageFactory.h"

#include "c74_min.h"

#include <string>
#include <utility>

namespace consolidator::maxadapter {

class ComponentOutputs final {
public:
    ComponentOutputs(
        c74::min::outlet<>* commands,
        c74::min::outlet<>* status,
        c74::min::outlet<>* debug
    ) : commands(commands), status(status), debug(debug) {}

    template <typename Message, typename... Arguments>
    bool Send(
        const std::string& source,
        const std::string& target,
        Arguments&&... arguments
    ) const {
        if (!commands) return false;
        const auto envelope = messaging::MessageFactory::Create<Message>(
            source, target, std::forward<Arguments>(arguments)...);
        MaxDictionarySerializer::Serialize(envelope, [this](const c74::min::atom& dictionary) {
            commands->send("message", dictionary);
        });
        return true;
    }

    void Status(application::ComponentStatus value) const {
        if (status) status->send("status", application::ComponentStatusName(value).data());
    }

    void Ready(bool value) const {
        if (status) status->send("ready", value ? 1 : 0);
    }

    template <typename... Arguments>
    void Debug(const char* selector, Arguments&&... arguments) const {
        if (debug) debug->send(selector, std::forward<Arguments>(arguments)...);
    }

    template <typename... Arguments>
    void Error(Arguments&&... arguments) const {
        Debug("error", std::forward<Arguments>(arguments)...);
    }

private:
    c74::min::outlet<>* commands;
    c74::min::outlet<>* status;
    c74::min::outlet<>* debug;
};

} // namespace consolidator::maxadapter
