#include "Shared/Protocol/ProtocolFrame.h"

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace consolidator::vst3::shared::protocol
{

std::vector<max::transport::NativeAtom> EncodeAtoms(const std::vector<ProtocolValue>& values)
{
    std::vector<max::transport::NativeAtom> atoms;
    atoms.reserve(values.size());
    for (const auto& value : values)
    {
        max::transport::NativeAtom atom{};
        std::visit([&atom](const auto& item)
        {
            using Value = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<Value, std::int64_t>)
            {
                atom.type = max::transport::NativeAtomType::Integer;
                atom.integer = item;
            }
            else if constexpr (std::is_same_v<Value, double>)
            {
                atom.type = max::transport::NativeAtomType::Float;
                atom.floating = item;
            }
            else
            {
                atom.type = max::transport::NativeAtomType::Symbol;
                atom.symbol = item.c_str();
            }
        }, value);
        atoms.push_back(atom);
    }
    return atoms;
}

ProtocolFrame CopyFrame(const char* selector, const max::transport::NativeAtom* atoms, std::size_t count)
{
    if (selector == nullptr || (count != 0 && atoms == nullptr))
    {
        throw std::invalid_argument("Invalid Managed callback frame.");
    }
    ProtocolFrame frame{ selector, {} };
    frame.values.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto& atom = atoms[index];
        switch (atom.type)
        {
        case max::transport::NativeAtomType::Integer:
        {
            frame.values.emplace_back(atom.integer);
            break;
        }
        case max::transport::NativeAtomType::Float:
        {
            frame.values.emplace_back(atom.floating);
            break;
        }
        case max::transport::NativeAtomType::Symbol:
        {
            if (atom.symbol == nullptr)
            {
                throw std::invalid_argument("Null Managed callback symbol.");
            }
            frame.values.emplace_back(std::string(atom.symbol));
            break;
        }
        default:
        {
            throw std::invalid_argument("Unknown Managed callback atom type.");
        }
        }
    }
    return frame;
}

double Number(const ProtocolValue& value)
{
    if (const auto integer = std::get_if<std::int64_t>(&value))
    {
        return static_cast<double>(*integer);
    }
    return std::get<double>(value);
}

const std::string& Symbol(const ProtocolValue& value)
{
    return std::get<std::string>(value);
}

std::uint64_t WireId(const ProtocolValue& value)
{
    const auto& text = Symbol(value);
    std::uint64_t id = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), id);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    {
        throw std::invalid_argument("Invalid protocol wire ID.");
    }
    return id;
}

std::int64_t Integer(const ProtocolValue& value)
{
    return std::get<std::int64_t>(value);
}

bool Boolean(const ProtocolValue& value)
{
    const auto integer = Integer(value);
    if (integer != 0 && integer != 1)
    {
        throw std::invalid_argument("Invalid protocol boolean.");
    }
    return integer != 0;
}

std::optional<double> OptionalNumber(const ProtocolValue& value)
{
    if (const auto symbol = std::get_if<std::string>(&value); symbol != nullptr && *symbol == "none")
    {
        return {};
    }
    const auto number = Number(value);
    if (!std::isfinite(number))
    {
        throw std::invalid_argument("Non-finite parameter metadata.");
    }
    return number;
}

std::vector<ProtocolValue> EncodePath(const std::string& path)
{
    std::vector<ProtocolValue> values;
    std::size_t begin = 0;
    while (begin < path.size())
    {
        const auto end = path.find('.', begin);
        const auto part = path.substr(begin, end == std::string::npos ? end : end - begin);
        std::int64_t index = 0;
        const auto parsed = std::from_chars(part.data(), part.data() + part.size(), index);
        if (parsed.ec == std::errc{} && parsed.ptr == part.data() + part.size())
        {
            values.emplace_back(index);
        }
        else
        {
            values.emplace_back(part);
        }
        if (end == std::string::npos)
        {
            break;
        }
        begin = end + 1;
    }
    return values;
}

std::vector<ProtocolValue> EncodeStateWrite(
    const std::vector<StateWriteEntry>& entries, const std::string& scope, const std::string& transaction)
{
    std::vector<ProtocolValue> body{ scope, transaction, static_cast<std::int64_t>(entries.size()) };
    for (const auto& entry : entries)
    {
        // Same relativePath contract as the existing Max TargetStateClient.
        const auto writePath = entry.path.starts_with("equalizer.filter.")
            ? "equalizer.bank." + entry.path.substr(std::string("equalizer.").size()) : entry.path;
        body.emplace_back(std::string("entry"));
        const auto parts = EncodePath(writePath);
        body.insert(body.end(), parts.begin(), parts.end());
        body.insert(body.end(), { std::string("value"), entry.value, entry.mode });
    }
    return body;
}

} // namespace consolidator::vst3::shared::protocol
