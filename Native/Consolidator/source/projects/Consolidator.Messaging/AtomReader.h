#pragma once

#include "AtomTypes.h"

#include <optional>
#include <type_traits>

namespace consolidator::messaging {

class AtomReader final {
public:
    explicit AtomReader(const AtomList& atoms) : atoms(atoms) {}

    std::optional<std::int64_t> ReadInt() {
        const auto value = ReadValue();
        if (!value) return std::nullopt;
        if (const auto integer = std::get_if<std::int64_t>(&*value)) return *integer;
        if (const auto boolean = std::get_if<bool>(&*value)) return *boolean ? 1 : 0;
        return std::nullopt;
    }

    std::optional<double> ReadDouble() {
        const auto value = ReadValue();
        if (!value) return std::nullopt;
        if (const auto number = std::get_if<double>(&*value)) return *number;
        if (const auto integer = std::get_if<std::int64_t>(&*value)) return static_cast<double>(*integer);
        return std::nullopt;
    }

    std::optional<bool> ReadBool() {
        const auto value = ReadValue();
        if (!value) return std::nullopt;
        if (const auto boolean = std::get_if<bool>(&*value)) return *boolean;
        if (const auto integer = std::get_if<std::int64_t>(&*value)) {
            if (*integer == 0 || *integer == 1) return *integer == 1;
        }
        return std::nullopt;
    }

    std::optional<std::string> ReadString() {
        const auto value = ReadValue();
        if (!value) return std::nullopt;
        if (const auto text = std::get_if<std::string>(&*value)) return *text;
        return std::nullopt;
    }

    bool RequireEnd() const noexcept { return index == atoms.size(); }
    std::size_t Index() const noexcept { return index; }

private:
    std::optional<AtomValue> ReadValue() {
        if (index >= atoms.size()) return std::nullopt;
        return atoms[index++];
    }

    const AtomList& atoms;
    std::size_t index = 0;
};

} // namespace consolidator::messaging
