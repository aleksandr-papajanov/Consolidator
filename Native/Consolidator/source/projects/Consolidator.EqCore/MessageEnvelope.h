#pragma once

#include "c74_min.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace consolidator::protocol {

// Common structured message envelope shared by all non-audio components.
class MessageEnvelope {
public:
    explicit MessageEnvelope(const c74::min::atom& value)
        : data_(value) {}

    explicit MessageEnvelope(const std::string& type)
        : data_{ next_dictionary_name() }, payload_{} {
        data_["type"] = type;
        auto* data_object = static_cast<c74::max::t_object*>(data_);
        auto* payload_object = static_cast<c74::max::t_object*>(payload_);
        c74::max::dictionary_appenddictionary(
            reinterpret_cast<c74::max::t_dictionary*>(data_object),
            c74::max::gensym("payload"),
            payload_object);
    }

    static std::unique_ptr<MessageEnvelope> from_atom(const c74::min::atom& value) {
        try {
            if (c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
                    static_cast<const c74::max::t_atom*>(&value)))) {
                return std::unique_ptr<MessageEnvelope>(new MessageEnvelope(value));
            }

            const auto name = static_cast<std::string>(value);
            c74::min::dict named{ c74::min::symbol(name.c_str()) };
            auto message = std::unique_ptr<MessageEnvelope>(
                new MessageEnvelope(c74::min::atom{
                    static_cast<c74::max::t_object*>(named) }));
            std::string type;
            return message->type(type) ? std::move(message) : nullptr;
        }
        catch (...) {
            return nullptr;
        }
    }

    bool type(std::string& value) const {
        return read_symbol("type", value);
    }

    bool target(std::string& value) const {
        return read_symbol("target", value);
    }

    bool source(std::string& value) const {
        return read_symbol("source", value);
    }

    bool is_addressed_to(const char* feature) const {
        std::string value;
        return target(value) && (value == feature || value == "broadcast");
    }

    bool has(const char* key) const {
        try {
            auto* object = static_cast<c74::max::t_object*>(data_);
            return c74::max::dictionary_hasentry(
                reinterpret_cast<c74::max::t_dictionary*>(object),
                c74::max::gensym(key));
        }
        catch (...) {
            return false;
        }
    }

    bool payload(c74::min::dict& value) const {
        if (!has("payload")) {
            return false;
        }
        try {
            value = c74::min::dict{ static_cast<c74::min::atom>(mutable_data().at("payload")) };
            return value.valid();
        }
        catch (...) {
            return false;
        }
    }

    bool payload_number(const char* key, double& value) const {
        c74::min::dict payload_data;
        if (!payload(payload_data)) {
            return false;
        }
        try {
            value = static_cast<double>(static_cast<c74::min::atom>(payload_data.at(key)));
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool payload_symbol(const char* key, std::string& value) const {
        c74::min::dict payload_data;
        if (!payload(payload_data)) {
            return false;
        }
        try {
            value = static_cast<std::string>(static_cast<c74::min::atom>(payload_data.at(key)));
            return !value.empty();
        }
        catch (...) {
            return false;
        }
    }

    bool payload_numbers(const char* key, std::vector<double>& values) const {
        c74::min::dict payload_data;
        if (!payload(payload_data)) {
            return false;
        }
        try {
            const auto numbers = static_cast<std::vector<c74::min::number>>(
                payload_data.at(key));
            values.assign(numbers.begin(), numbers.end());
            return !values.empty();
        }
        catch (...) {
            return false;
        }
    }

    bool payload_dictionary(const char* key, c74::min::atom& value) const {
        c74::min::dict payload_data;
        if (!payload(payload_data)) {
            return false;
        }
        try {
            const auto candidate = static_cast<c74::min::atom>(payload_data.at(key));
            if (!c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
                    static_cast<const c74::max::t_atom*>(&candidate)))) {
                return false;
            }
            value = candidate;
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void set_target(const std::string& value) {
        data_["target"] = value;
    }

    void set_source(const std::string& value) {
        data_["source"] = value;
    }

    void set_payload_number(const char* key, const double value) {
        payload_[key] = value;
    }

    void set_payload_symbol(const char* key, const std::string& value) {
        payload_[key] = value;
    }

    void set_payload_numbers(const char* key, const std::vector<double>& values) {
        c74::min::atoms atoms;
        atoms.reserve(values.size());
        for (const auto value : values) {
            atoms.push_back(value);
        }
        auto* object = static_cast<c74::max::t_object*>(payload_);
        c74::max::dictionary_appendatoms(
            reinterpret_cast<c74::max::t_dictionary*>(object),
            c74::max::gensym(key),
            static_cast<long>(atoms.size()),
            atoms.empty() ? nullptr : &atoms[0]);
    }

    void set_payload_dictionary(const char* key, const c74::min::atom& value) {
        if (!c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
                static_cast<const c74::max::t_atom*>(&value)))) {
            return;
        }
        auto* dictionary_object = static_cast<c74::max::t_object*>(c74::max::atom_getobj(
            const_cast<c74::max::t_atom*>(
                static_cast<const c74::max::t_atom*>(&value))));
        auto* payload_object = static_cast<c74::max::t_object*>(payload_);
        c74::max::dictionary_appenddictionary(
            reinterpret_cast<c74::max::t_dictionary*>(payload_object),
            c74::max::gensym(key),
            dictionary_object);
    }

    c74::min::atom as_atom() const {
        return c74::min::atom{ static_cast<c74::max::t_object*>(data_) };
    }

    c74::min::atom transport_atom() const {
        return c74::min::atom{ data_.name() };
    }

    c74::min::dict& data() { return data_; }
    const c74::min::dict& data() const { return data_; }

private:
    static c74::min::symbol next_dictionary_name() {
        static std::atomic<unsigned long> sequence{ 0 };
        const auto name = std::string{ "consolidator.message." } +
            std::to_string(++sequence);
        return c74::min::symbol{ name.c_str() };
    }

    bool read_symbol(const char* key, std::string& value) const {
        try {
            value = static_cast<std::string>(static_cast<c74::min::atom>(mutable_data().at(key)));
            return !value.empty();
        }
        catch (...) {
            return false;
        }
    }

    bool read_long(const char* key, long& value) const {
        try {
            value = static_cast<long>(static_cast<c74::min::atom>(mutable_data().at(key)));
            return true;
        }
        catch (...) {
            return false;
        }
    }

    c74::min::dict& mutable_data() const {
        return const_cast<c74::min::dict&>(data_);
    }

    c74::min::dict data_;
    c74::min::dict payload_;
};

} // namespace consolidator::protocol
