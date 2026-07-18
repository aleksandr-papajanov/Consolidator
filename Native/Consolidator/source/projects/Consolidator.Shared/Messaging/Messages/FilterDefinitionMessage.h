#pragma once

#include "IMessage.h"

#include <string>
#include <utility>
#include <vector>

namespace consolidator::messaging {

class FilterDefinitionMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "filter.define";

    FilterDefinitionMessage(
        long filterId,
        std::string contractName,
        std::vector<double> defaultValues = {},
        bool defaultBypass = false
    ) : filterId(filterId), contractName(std::move(contractName)),
        defaultValues(std::move(defaultValues)), defaultBypass(defaultBypass) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("filterId", static_cast<std::int64_t>(filterId));
        payload.Set("contractName", contractName);
        payload.Set("defaultBypass", defaultBypass);
        MessageArray values;
        values.reserve(defaultValues.size());
        for (const auto value : defaultValues) values.emplace_back(value);
        payload.Set("defaultValues", std::move(values));
        return payload;
    }

    static std::optional<FilterDefinitionMessage> Deserialize(const MessagePayload& payload) {
        const auto filterId = payload.ReadLong("filterId");
        const auto contractName = payload.ReadString("contractName");
        if (!filterId || !contractName) return std::nullopt;

        std::vector<double> values;
        if (const auto source = payload.ReadArray("defaultValues")) {
            values.reserve(source->size());
            for (const auto& value : *source) {
                if (const auto number = value.As<double>()) values.push_back(*number);
                else if (const auto integer = value.As<std::int64_t>()) {
                    values.push_back(static_cast<double>(*integer));
                }
                else return std::nullopt;
            }
        }
        return FilterDefinitionMessage{
            *filterId, *contractName, std::move(values), payload.ReadBool("defaultBypass").value_or(false) };
    }

    long filterId;
    std::string contractName;
    std::vector<double> defaultValues;
    bool defaultBypass;
};

} // namespace consolidator::messaging
