#pragma once

#include "DeviceStateDictionaryCodec.h"
#include "MessageEnvelopeDictionaryCodec.h"

#include "c74_min.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace consolidator::maxadapter {

class MaxDictionarySerializer final {
public:
    template <typename Type>
    static std::optional<Type> Deserialize(const c74::min::atom& atom) {
        const auto object = Read(atom);
        return object ? DictionaryCodec<Type>::Deserialize(*object) : std::nullopt;
    }

    template <typename Type>
    static std::optional<Type> Deserialize(const std::string& dictionaryName) {
        return Deserialize<Type>(c74::min::atom{
            c74::min::symbol{ dictionaryName.c_str() }
        });
    }

    template <typename Type, typename Sender>
    static void Serialize(const Type& value, Sender&& sender) {
        const auto name = NextDictionaryName();
        c74::min::dict dictionary{ name };
        WriteObject(dictionary, DictionaryCodec<Type>::Serialize(value));
        sender(c74::min::atom{ dictionary.name() });
    }

private:
    static std::optional<messaging::MessageObject> Read(const c74::min::atom& atom) {
        try {
            auto dictionary = OpenDictionary(atom);
            return ReadObject(dictionary);
        }
        catch (...) {
            return std::nullopt;
        }
    }

    static c74::min::dict OpenDictionary(const c74::min::atom& atom) {
        if (c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
                static_cast<const c74::max::t_atom*>(&atom)))) {
            return c74::min::dict{ atom };
        }
        return c74::min::dict{ c74::min::symbol{ static_cast<std::string>(atom).c_str() } };
    }

    static messaging::MessageObject ReadObject(c74::min::dict& dictionary) {
        messaging::MessageObject object;
        for (const auto& key : dictionary.keys()) {
            const std::string name = static_cast<const char* const>(key);
            const auto atoms = static_cast<c74::min::atoms>(dictionary.at(key));
            if (atoms.size() == 1) object[name] = ReadAtom(atoms.front());
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
        for (const auto& [key, value] : object) {
            if (!value.As<messaging::MessageObject>()) WriteValue(dictionary, key, value);
        }
        for (const auto& [key, value] : object) {
            if (value.As<messaging::MessageObject>()) WriteValue(dictionary, key, value);
        }
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
            auto* nestedObject = c74::max::dictionary_new();
            c74::min::dict nested{ nestedObject, false };
            WriteObject(nested, *object);
            AppendDictionary(dictionary, key, nestedObject);
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
        c74::max::t_dictionary* value
    ) {
        auto* destinationObject = static_cast<c74::max::t_object*>(destination);
        const auto error = c74::max::dictionary_appenddictionary(
            reinterpret_cast<c74::max::t_dictionary*>(destinationObject),
            c74::max::gensym(key.c_str()), reinterpret_cast<c74::max::t_object*>(value));
        c74::max::object_release(destinationObject);
        if (error != c74::max::MAX_ERR_NONE) {
            c74::max::object_free(value);
            throw std::runtime_error("Could not append nested Max dictionary");
        }
    }

    static c74::min::symbol NextDictionaryName() {
        static std::atomic<unsigned long> sequence{ 0 };
        const auto name = std::string{ "consolidator.dictionary." } + std::to_string(++sequence);
        return c74::min::symbol{ name.c_str() };
    }
};

} // namespace consolidator::maxadapter
