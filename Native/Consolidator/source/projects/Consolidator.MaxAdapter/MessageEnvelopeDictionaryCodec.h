#pragma once

#include "DictionaryCodec.h"
#include "Messaging/MessageEnvelope.h"

namespace consolidator::maxadapter {

template <>
struct DictionaryCodec<messaging::MessageEnvelope> final {
    static std::optional<messaging::MessageEnvelope> Deserialize(
        const messaging::MessageObject& object
    ) {
        const messaging::MessagePayload root{ object };
        const auto type = root.ReadString("type");
        const auto source = root.ReadString("source");
        const auto target = root.ReadString("target");
        const auto payload = root.ReadObject("payload");
        if (!type || !source || !target || !payload) return std::nullopt;
        return messaging::MessageEnvelope{
            *type, *source, *target, messaging::MessagePayload{ *payload }
        };
    }

    static messaging::MessageObject Serialize(const messaging::MessageEnvelope& envelope) {
        return {
            { "type", envelope.type },
            { "source", envelope.source },
            { "target", envelope.target },
            { "payload", envelope.payload.Values() }
        };
    }
};

} // namespace consolidator::maxadapter
