#pragma once

#include "Application/ComponentRouter.h"
#include "ComponentOutputs.h"
#include "MaxDictionarySerializer.h"
#include "Messaging/Messages/DeviceStateChangedMessage.h"
#include "Models/DeviceState.h"

#include "c74_min.h"

#include <limits>
#include <string>

namespace consolidator::maxadapter {

template <typename Owner, typename... Messages>
class ComponentHost final {
public:
    ComponentHost(
        Owner& owner,
        std::string address,
        c74::min::outlet<>* commands = nullptr,
        c74::min::outlet<>* status = nullptr,
        c74::min::outlet<>* debug = nullptr
    ) : owner(owner),
        address(std::move(address)),
        router(owner),
        outputs(commands, status, debug) {}

    bool Receive(const c74::min::atoms& arguments) {
        if (arguments.size() != 1) {
            outputs.Error("invalid_message_envelope");
            return false;
        }
        const auto envelope = MaxDictionarySerializer::Deserialize<
            messaging::MessageEnvelope>(arguments[0]);
        if (!envelope) {
            outputs.Error("invalid_message_envelope");
            return false;
        }
        if (envelope->target != address) return false;
        if (envelope->type == messaging::DeviceStateChangedMessage::TypeName) {
            return ReceiveDeviceState(*envelope);
        }

        const auto result = router.Route(*envelope);
        if (result == application::RouteResult::InvalidPayload) {
            outputs.Error("invalid_message_payload", envelope->type);
            return false;
        }
        if (result == application::RouteResult::NotMatched) {
            outputs.Error("unsupported_message", envelope->type);
            return false;
        }
        return true;
    }

    ComponentOutputs& Outputs() {
        return outputs;
    }

    const std::string& Address() const {
        return address;
    }

private:
    bool ReceiveDeviceState(const messaging::MessageEnvelope& envelope) {
        const auto changed = messaging::DeviceStateChangedMessage::Deserialize(envelope.payload);
        if (!changed) {
            outputs.Error("invalid_device_state_message");
            return false;
        }
        if (changed->generation <= generation) return true;
        const auto state = MaxDictionarySerializer::Deserialize<models::DeviceState>(
            changed->stateName);
        if (!state || state->generation != changed->generation) {
            outputs.Error("invalid_device_state");
            return false;
        }
        generation = changed->generation;
        owner.OnDeviceStateChanged(*state);
        return true;
    }

    Owner& owner;
    std::string address;
    application::ComponentRouter<Owner, Messages...> router;
    ComponentOutputs outputs;
    long generation = std::numeric_limits<long>::min();
};

} // namespace consolidator::maxadapter
