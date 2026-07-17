#pragma once

#include "MessageEnvelope.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace consolidator::protocol {

enum class MessageDispatchResult { unmatched, invalid, handled };

template <typename Message>
std::optional<Message> deserialize(const MessageEnvelope& envelope) {
    std::string type;
    if (!envelope.type(type) || type != Message::type) {
        return std::nullopt;
    }
    return Message::from_envelope(envelope);
}

template <typename Message, typename Handler>
MessageDispatchResult dispatch_one(const MessageEnvelope& envelope, Handler&& handler) {
    std::string type;
    if (!envelope.type(type) || type != Message::type) {
        return MessageDispatchResult::unmatched;
    }
    const auto message = Message::from_envelope(envelope);
    if (!message) {
        return MessageDispatchResult::invalid;
    }
    handler(*message);
    return MessageDispatchResult::handled;
}

template <typename First, typename... Rest, typename Handler>
MessageDispatchResult dispatch(const MessageEnvelope& envelope, Handler&& handler) {
    const auto result = dispatch_one<First>(envelope, handler);
    if (result != MessageDispatchResult::unmatched) {
        return result;
    }
    if constexpr (sizeof...(Rest) == 0) {
        return MessageDispatchResult::unmatched;
    }
    else {
        return dispatch<Rest...>(envelope, std::forward<Handler>(handler));
    }
}

struct FilterDefineMessage {
    static constexpr const char* type = "filter.define";
    long filterId{};
    c74::min::atom contract;
    std::string contractName;

    static std::optional<FilterDefineMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterDefineMessage result;
        double filterId{};
        if (!envelope.payload_number("filterId", filterId)) return std::nullopt;
        result.filterId = static_cast<long>(filterId);
        if (envelope.payload_symbol("contractName", result.contractName)) return result;
        return envelope.payload_dictionary("contract", result.contract)
            ? std::optional<FilterDefineMessage>{ result }
            : std::nullopt;
    }

      MessageEnvelope to_envelope() const {
          MessageEnvelope envelope{ std::string{ type } };
          envelope.set_target("eq.storage");
          envelope.set_source("filter");
          envelope.set_payload_number("filterId", filterId);
          if (!contractName.empty()) {
              envelope.set_payload_symbol("contractName", contractName);
          }
          else {
              envelope.set_payload_dictionary("contract", contract);
          }
          return envelope;
      }
};

struct FilterUpdateMessage {
    static constexpr const char* type = "filter.update";
    long filterId{};
    std::vector<double> values;
    std::optional<long> bankIndex;

    static std::optional<FilterUpdateMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterUpdateMessage result;
        double filterId{};
        if (!envelope.payload_number("filterId", filterId) ||
            !envelope.payload_numbers("values", result.values)) {
            return std::nullopt;
        }
        result.filterId = static_cast<long>(filterId);
        double bankIndex{};
        if (envelope.payload_number("bankIndex", bankIndex)) {
            result.bankIndex = static_cast<long>(bankIndex);
        }
        return result;
    }

    MessageEnvelope to_envelope() const {
        MessageEnvelope envelope{ std::string{ type } };
          envelope.set_target("eq.storage");
          envelope.set_source("filter");
        envelope.set_payload_number("filterId", filterId);
        envelope.set_payload_numbers("values", values);
        if (bankIndex) {
            envelope.set_payload_number("bankIndex", *bankIndex);
        }
        return envelope;
    }
};

struct FilterBypassMessage {
    static constexpr const char* type = "filter.bypass";
    long filterId{};
    bool bypassed{};

    static std::optional<FilterBypassMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterBypassMessage result;
        double value{};
        double filterId{};
        if (!envelope.payload_number("filterId", filterId) || !envelope.payload_number("value", value) ||
            (value != 0.0 && value != 1.0)) {
            return std::nullopt;
        }
        result.filterId = static_cast<long>(filterId);
        result.bypassed = value == 1.0;
        return result;
    }

    MessageEnvelope to_envelope() const {
        MessageEnvelope envelope{ std::string{ type } };
        envelope.set_target("eq.storage");
        envelope.set_source("filter");
        envelope.set_payload_number("filterId", filterId);
        envelope.set_payload_number("value", bypassed ? 1.0 : 0.0);
        return envelope;
    }
};

struct FilterControlUpdateMessage {
    static constexpr const char* type = "filter.control.update";
    long filterId{};
    std::string control;
    double value{};

    static std::optional<FilterControlUpdateMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterControlUpdateMessage result;
        double filterId{};
        if (!envelope.payload_number("filterId", filterId) ||
            !envelope.payload_symbol("control", result.control) ||
            !envelope.payload_number("value", result.value)) {
            return std::nullopt;
        }
        result.filterId = static_cast<long>(filterId);
        return result;
    }
};

struct FilterInstanceStateMessage {
    static constexpr const char* type = "filter.instance.state";
    long filterId{};
    bool recovered{};

    static std::optional<FilterInstanceStateMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterInstanceStateMessage result;
        double value{};
        double filterId{};
        if (!envelope.payload_number("filterId", filterId) || !envelope.payload_number("recovered", value) ||
            (value != 0.0 && value != 1.0)) {
            return std::nullopt;
        }
        result.filterId = static_cast<long>(filterId);
        result.recovered = value == 1.0;
        return result;
    }
};

struct FilterResetMessage {
    static constexpr const char* type = "filter.reset";
    long filterId{};
    static std::optional<FilterResetMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterResetMessage result;
        double filterId{};
        if (!envelope.payload_number("filterId", filterId)) return std::nullopt;
        result.filterId = static_cast<long>(filterId);
        return result;
    }
};

struct FilterEditMessage {
    static constexpr const char* type = "filter.edit";
    long filterId{};
    std::optional<double> frequency;
    std::optional<double> gain;
    std::optional<double> q;

    static std::optional<FilterEditMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterEditMessage result;
        double filterId{};
        if (!envelope.payload_number("filterId", filterId)) return std::nullopt;
        result.filterId = static_cast<long>(filterId);
        std::string parameter;
        double value{};
        if (envelope.payload_symbol("parameter", parameter) && envelope.payload_number("value", value)) {
            if (parameter != "q" || value < 0.0 || value > 1.0) return std::nullopt;
            result.q = value;
            return result;
        }
        double frequency{};
        double gain{};
        if (!envelope.payload_number("frequency", frequency) || !envelope.payload_number("gain", gain)) {
            return std::nullopt;
        }
        result.frequency = frequency;
        result.gain = gain;
        return result;
    }
};

struct EqStorageSnapshotMessage {
    static constexpr const char* type = "eq.storage.snapshot";
    std::string snapshotName;

    static std::optional<EqStorageSnapshotMessage> from_envelope(const MessageEnvelope& envelope) {
        EqStorageSnapshotMessage result;
        return envelope.payload_symbol("snapshotName", result.snapshotName)
            ? std::optional<EqStorageSnapshotMessage>{ std::move(result) }
            : std::nullopt;
    }
};

struct ApproximatorClearMessage {
    static constexpr const char* type = "approximator.clear";
    static std::optional<ApproximatorClearMessage> from_envelope(const MessageEnvelope&) { return ApproximatorClearMessage{}; }
};

struct EqStorageBankChangedMessage {
    static constexpr const char* type = "eq.storage.bank.changed";
    long bankIndex{};

    static std::optional<EqStorageBankChangedMessage> from_envelope(
        const MessageEnvelope& envelope
    ) {
        double bankIndex{};
        if (!envelope.payload_number("bankIndex", bankIndex)) {
            return std::nullopt;
        }
        return EqStorageBankChangedMessage{ static_cast<long>(bankIndex) };
    }
};

struct ApproximatorFitMessage {
    static constexpr const char* type = "approximator.fit";
    static std::optional<ApproximatorFitMessage> from_envelope(const MessageEnvelope&) { return ApproximatorFitMessage{}; }
};

struct AnalyzerDifferenceMessage {
    static constexpr const char* type = "analyzer.difference";
    bool enabled{};
    static std::optional<AnalyzerDifferenceMessage> from_envelope(const MessageEnvelope& envelope) {
        double value{};
        if (!envelope.payload_number("value", value) || (value != 0.0 && value != 1.0)) return std::nullopt;
        return AnalyzerDifferenceMessage{ value == 1.0 };
    }
};

struct AnalyzerStatsMessage {
    static constexpr const char* type = "analyzer.stats";
    static std::optional<AnalyzerStatsMessage> from_envelope(const MessageEnvelope&) { return AnalyzerStatsMessage{}; }
};

} // namespace consolidator::protocol
