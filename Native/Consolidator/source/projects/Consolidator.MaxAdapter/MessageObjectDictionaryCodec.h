#pragma once

#include "DictionaryCodec.h"
#include "Messaging/MessagePayload.h"

namespace consolidator::maxadapter {

template <>
struct DictionaryCodec<messaging::MessageObject> final {
    static std::optional<messaging::MessageObject> Deserialize(
        const messaging::MessageObject& object
    ) {
        return object;
    }

    static messaging::MessageObject Serialize(const messaging::MessageObject& object) {
        return object;
    }
};

} // namespace consolidator::maxadapter
