#pragma once

#include "MessageEnvelope.h"
#include "IMessageDecoder.h"

#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace consolidator::messaging {

class MessageFactory {
public:
    template <typename Message, typename... Args>
    static MessageEnvelope Create(std::string source, std::string target, Args&&... args) {
        Message message{ std::forward<Args>(args)... };
        return { std::string{ message.Type() }, std::move(source), std::move(target), message.Serialize() };
    }

    template <typename Message>
    void Register() {
        decoders[Message::TypeName] = std::make_unique<MessageDecoder<Message>>();
    }

    std::unique_ptr<IMessage> Deserialize(const MessageEnvelope& envelope) const {
        const auto decoder = decoders.find(envelope.type);
        return decoder == decoders.end() ? nullptr : decoder->second->Deserialize(envelope.payload);
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IMessageDecoder>> decoders;
};

} // namespace consolidator::messaging
