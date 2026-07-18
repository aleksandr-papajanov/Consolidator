#pragma once

#include "Messaging/MessageEnvelope.h"

namespace consolidator::application {

enum class RouteResult {
    NotMatched,
    Handled,
    InvalidPayload
};

template <typename Owner, typename... Messages>
class ComponentRouter final {
public:
    explicit ComponentRouter(Owner& owner)
        : owner(owner) {}

    RouteResult Route(const messaging::MessageEnvelope& envelope) {
        RouteResult result = RouteResult::NotMatched;
        (TryRoute<Messages>(envelope, result), ...);
        return result;
    }

private:
    template <typename Message>
    void TryRoute(
        const messaging::MessageEnvelope& envelope,
        RouteResult& result
    ) {
        if (result != RouteResult::NotMatched || envelope.type != Message::TypeName) return;
        const auto message = Message::Deserialize(envelope.payload);
        if (!message) {
            result = RouteResult::InvalidPayload;
            return;
        }
        owner.OnMessage(*message);
        result = RouteResult::Handled;
    }

    Owner& owner;
};

} // namespace consolidator::application
