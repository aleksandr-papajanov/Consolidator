#pragma once

#include "MessageEnvelope.h"

#include <utility>

namespace consolidator::messaging {

class MessageFactory {
public:
    template <typename Message, typename... Args>
    static MessageEnvelope Create(std::string source, std::string target, Args&&... args) {
        Message message{ std::forward<Args>(args)... };
        return { std::string{ message.Type() }, std::move(source), std::move(target), message.Serialize() };
    }
};

} // namespace consolidator::messaging
