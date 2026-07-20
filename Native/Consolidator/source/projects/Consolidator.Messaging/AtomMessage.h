#pragma once

#include "AtomTypes.h"

#include <optional>
#include <string>
#include <string_view>

namespace consolidator::messaging {

class AtomMessage final {
public:
    static bool HasCategory(const std::optional<AtomList>& atoms, std::string_view category) {
        if (!atoms || atoms->empty()) return false;
        const auto value = std::get_if<std::string>(&atoms->front());
        return value && *value == category;
    }

    static bool HasSnapshotStore(const std::optional<AtomList>& atoms, std::string_view store) {
        if (!HasCategory(atoms, "snapshot") || atoms->size() < 4) return false;
        const auto value = std::get_if<std::string>(&(*atoms)[3]);
        return value && *value == store;
    }
};

} // namespace consolidator::messaging
