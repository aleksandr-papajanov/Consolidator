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

struct FilterApplyMessage {
    static constexpr const char* type = "filter.apply";
    long filterId{};
    std::vector<double> values;
    std::optional<long> bankIndex;

    static std::optional<FilterApplyMessage> from_envelope(const MessageEnvelope& envelope) {
        FilterApplyMessage result;
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
        envelope.set_target("filter");
        envelope.set_source("approximator");
        envelope.set_payload_number("filterId", filterId);
        envelope.set_payload_numbers("values", values);
        if (bankIndex) envelope.set_payload_number("bankIndex", *bankIndex);
        return envelope;
    }
};

struct EqStorageSnapshotMessage {
    static constexpr const char* type = "eq.storage.snapshot";
    std::string snapshotName;
    long selectedBankId{};

    static std::optional<EqStorageSnapshotMessage> from_envelope(const MessageEnvelope& envelope) {
        EqStorageSnapshotMessage result;
        double selectedBankId{};
        if (!envelope.payload_symbol("snapshotName", result.snapshotName) ||
            !envelope.payload_number("selectedBankId", selectedBankId)) {
            return std::nullopt;
        }
        result.selectedBankId = static_cast<long>(selectedBankId);
        return result;
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
