#pragma once

#include "Messages/IMessage.h"

#include <memory>
#include <string_view>

namespace consolidator::messaging {

class IMessageDecoder {
public:
    virtual ~IMessageDecoder() = default;

    virtual std::string_view Type() const = 0;
    virtual std::unique_ptr<IMessage> Deserialize(const MessagePayload& payload) const = 0;
};

template <typename Message>
class MessageDecoder final : public IMessageDecoder {
public:
    std::string_view Type() const override {
        return Message::TypeName;
    }

    std::unique_ptr<IMessage> Deserialize(const MessagePayload& payload) const override {
        const auto message = Message::Deserialize(payload);
        return message ? std::make_unique<Message>(*message) : nullptr;
    }
};

} // namespace consolidator::messaging
