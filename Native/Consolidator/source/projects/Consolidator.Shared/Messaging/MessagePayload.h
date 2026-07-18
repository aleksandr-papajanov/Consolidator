#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace consolidator::messaging {

class MessageValue;
using MessageArray = std::vector<MessageValue>;
using MessageObject = std::map<std::string, MessageValue>;

class MessageValue {
public:
    using Value = std::variant<std::monostate, bool, std::int64_t, double, std::string, MessageArray, MessageObject>;

    MessageValue() : value(std::monostate{}) {}
    MessageValue(bool value) : value(value) {}
    MessageValue(std::int64_t value) : value(value) {}
    MessageValue(double value) : value(value) {}
    MessageValue(std::string value) : value(std::move(value)) {}
    MessageValue(const char* value) : value(std::string{ value }) {}
    MessageValue(MessageArray value) : value(std::move(value)) {}
    MessageValue(MessageObject value) : value(std::move(value)) {}

    template <typename Type>
    const Type* As() const {
        return std::get_if<Type>(&value);
    }

private:
    Value value;
};

class MessagePayload {
public:
    MessagePayload() = default;

    explicit MessagePayload(MessageObject values)
        : values(std::move(values)) {}

    void Set(std::string key, MessageValue value) {
        values[std::move(key)] = std::move(value);
    }

    const MessageValue* Find(const std::string& key) const {
        const auto item = values.find(key);
        return item == values.end() ? nullptr : &item->second;
    }

    std::optional<bool> ReadBool(const std::string& key) const {
        const auto value = Find(key);
        if (!value) return std::nullopt;
        if (const auto boolean = value->As<bool>()) return *boolean;
        if (const auto integer = value->As<std::int64_t>(); integer && (*integer == 0 || *integer == 1)) {
            return *integer == 1;
        }
        return std::nullopt;
    }

    std::optional<long> ReadLong(const std::string& key) const {
        const auto value = Find(key);
        if (!value || !value->As<std::int64_t>()) return std::nullopt;
        return static_cast<long>(*value->As<std::int64_t>());
    }

    std::optional<double> ReadDouble(const std::string& key) const {
        const auto value = Find(key);
        if (!value) return std::nullopt;
        if (const auto integer = value->As<std::int64_t>()) return static_cast<double>(*integer);
        return value->As<double>() ? std::optional<double>{ *value->As<double>() } : std::nullopt;
    }

    std::optional<std::string> ReadString(const std::string& key) const {
        const auto value = Find(key);
        return value && value->As<std::string>() ? std::optional<std::string>{ *value->As<std::string>() } : std::nullopt;
    }

    const MessageArray* ReadArray(const std::string& key) const {
        const auto value = Find(key);
        return value ? value->As<MessageArray>() : nullptr;
    }

    const MessageObject* ReadObject(const std::string& key) const {
        const auto value = Find(key);
        return value ? value->As<MessageObject>() : nullptr;
    }

    const MessageObject& Values() const {
        return values;
    }

private:
    MessageObject values;
};

} // namespace consolidator::messaging
