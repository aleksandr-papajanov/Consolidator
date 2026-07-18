#pragma once

#include "c74_min.h"
#include "Messaging/MessageEnvelope.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

namespace consolidator::maxadapter {

class MaxMessageAdapter final {
public:
    static std::optional<messaging::MessageEnvelope> Deserialize(const c74::min::atom& atom) {
        try {
            auto dictionary = OpenDictionary(atom);
            const auto type = ReadString(dictionary, "type");
            const auto source = ReadString(dictionary, "source");
            const auto target = ReadString(dictionary, "target");
            if (!type || !source || !target) return std::nullopt;

            c74::min::dict payload{ static_cast<c74::min::atom>(dictionary.at("payload")) };
            return messaging::MessageEnvelope{
                *type, *source, *target, messaging::MessagePayload{ ReadObject(payload) } };
        }
        catch (...) {
            return std::nullopt;
        }
    }

    static c74::min::atom Serialize(const messaging::MessageEnvelope& envelope) {
        const auto name = NextDictionaryName();
        c74::min::dict dictionary{ name };
        dictionary["type"] = envelope.type;
        dictionary["source"] = envelope.source;
        dictionary["target"] = envelope.target;
        c74::min::dict payload;
        WriteObject(payload, envelope.payload.Values());
        AppendDictionary(dictionary, "payload", payload);
        return c74::min::atom{ dictionary.name() };
    }

    static bool IsAddressedTo(const messaging::MessageEnvelope& envelope, const char* target) {
        return envelope.target == target || envelope.target == "broadcast";
    }

private:
    static c74::min::dict OpenDictionary(const c74::min::atom& atom) {
        if (c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
                static_cast<const c74::max::t_atom*>(&atom)))) {
            return c74::min::dict{ atom };
        }
        return c74::min::dict{ c74::min::symbol{ static_cast<std::string>(atom).c_str() } };
    }

    static std::optional<std::string> ReadString(c74::min::dict& dictionary, const char* key) {
        try {
            const auto value = static_cast<std::string>(static_cast<c74::min::atom>(dictionary.at(key)));
            return value.empty() ? std::nullopt : std::optional<std::string>{ value };
        }
        catch (...) {
            return std::nullopt;
        }
    }

    static messaging::MessageObject ReadObject(c74::min::dict& dictionary) {
        messaging::MessageObject object;
        for (const auto& key : dictionary.keys()) {
            const std::string name = static_cast<const char* const>(key);
            const auto atoms = static_cast<c74::min::atoms>(dictionary.at(key));
            if (atoms.size() == 1) {
                object[name] = ReadAtom(atoms.front());
            }
            else {
                messaging::MessageArray values;
                values.reserve(atoms.size());
                for (const auto& atom : atoms) values.push_back(ReadAtom(atom));
                object[name] = std::move(values);
            }
        }
        return object;
    }

    static messaging::MessageValue ReadAtom(const c74::min::atom& atom) {
        switch (c74::max::atom_gettype(&atom)) {
            case c74::max::A_LONG:
                return static_cast<std::int64_t>(c74::max::atom_getlong(&atom));
            case c74::max::A_FLOAT:
                return c74::max::atom_getfloat(&atom);
            case c74::max::A_SYM:
                return std::string{ c74::max::atom_getsym(&atom)->s_name };
            case c74::max::A_OBJ:
                if (c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
                        static_cast<const c74::max::t_atom*>(&atom)))) {
                    c74::min::dict nested{ atom };
                    return ReadObject(nested);
                }
                break;
            default:
                break;
        }
        return {};
    }

    static void WriteObject(c74::min::dict& dictionary, const messaging::MessageObject& object) {
        for (const auto& [key, value] : object) WriteValue(dictionary, key, value);
    }

    static void WriteValue(
        c74::min::dict& dictionary,
        const std::string& key,
        const messaging::MessageValue& value
    ) {
        if (const auto boolean = value.As<bool>()) dictionary[key] = *boolean ? 1L : 0L;
        else if (const auto integer = value.As<std::int64_t>()) dictionary[key] = static_cast<long>(*integer);
        else if (const auto number = value.As<double>()) dictionary[key] = *number;
        else if (const auto text = value.As<std::string>()) dictionary[key] = *text;
        else if (const auto array = value.As<messaging::MessageArray>()) WriteArray(dictionary, key, *array);
        else if (const auto object = value.As<messaging::MessageObject>()) {
            c74::min::dict nested;
            WriteObject(nested, *object);
            AppendDictionary(dictionary, key, nested);
        }
    }

    static void WriteArray(
        c74::min::dict& dictionary,
        const std::string& key,
        const messaging::MessageArray& values
    ) {
        c74::min::atoms atoms;
        atoms.reserve(values.size());
        for (const auto& value : values) {
            if (const auto boolean = value.As<bool>()) atoms.push_back(*boolean ? 1L : 0L);
            else if (const auto integer = value.As<std::int64_t>()) atoms.push_back(static_cast<long>(*integer));
            else if (const auto number = value.As<double>()) atoms.push_back(*number);
            else if (const auto text = value.As<std::string>()) atoms.push_back(*text);
        }
        auto* object = static_cast<c74::max::t_object*>(dictionary);
        c74::max::dictionary_appendatoms(
            reinterpret_cast<c74::max::t_dictionary*>(object),
            c74::max::gensym(key.c_str()), static_cast<long>(atoms.size()),
            atoms.empty() ? nullptr : &atoms[0]);
        c74::max::object_release(object);
    }

    static void AppendDictionary(
        c74::min::dict& destination,
        const std::string& key,
        c74::min::dict& value
    ) {
        auto* destinationObject = static_cast<c74::max::t_object*>(destination);
        auto* valueObject = static_cast<c74::max::t_object*>(value);
        c74::max::dictionary_appenddictionary(
            reinterpret_cast<c74::max::t_dictionary*>(destinationObject),
            c74::max::gensym(key.c_str()), valueObject);
        c74::max::object_release(destinationObject);
        c74::max::object_release(valueObject);
    }

    static c74::min::symbol NextDictionaryName() {
        static std::atomic<unsigned long> sequence{ 0 };
        const auto name = std::string{ "consolidator.message." } + std::to_string(++sequence);
        return c74::min::symbol{ name.c_str() };
    }
};

} // namespace consolidator::maxadapter
