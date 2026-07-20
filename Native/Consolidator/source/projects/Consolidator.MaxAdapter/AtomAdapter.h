#pragma once

#include "AtomTypes.h"

#include "c74_min.h"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace consolidator::maxadapter {

class AtomAdapter final {
public:
    static std::optional<messaging::AtomList> Read(const c74::min::atoms& atoms) {
        messaging::AtomList result;
        result.reserve(atoms.size());
        for (const auto& atom : atoms) {
            switch (c74::max::atom_gettype(&atom)) {
                case c74::max::A_LONG:
                    result.emplace_back(static_cast<std::int64_t>(c74::max::atom_getlong(&atom)));
                    break;
                case c74::max::A_FLOAT:
                    result.emplace_back(static_cast<double>(c74::max::atom_getfloat(&atom)));
                    break;
                case c74::max::A_SYM:
                    result.emplace_back(std::string{ c74::max::atom_getsym(&atom)->s_name });
                    break;
                default:
                    return std::nullopt;
            }
        }
        return result;
    }

    static c74::min::atoms Write(const messaging::AtomList& values) {
        c74::min::atoms result;
        result.reserve(values.size());
        for (const auto& value : values) {
            std::visit([&result](const auto& item) {
                using Value = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<Value, std::int64_t>) result.emplace_back(static_cast<long>(item));
                else if constexpr (std::is_same_v<Value, double>) result.emplace_back(item);
                else if constexpr (std::is_same_v<Value, bool>) result.emplace_back(item ? 1L : 0L);
                else if constexpr (std::is_same_v<Value, std::string>) result.emplace_back(item);
            }, value);
        }
        return result;
    }
};

} // namespace consolidator::maxadapter
