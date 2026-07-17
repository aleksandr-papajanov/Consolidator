#pragma once

#include "EqConstants.h"
#include "FilterContract.h"

#include <array>
#include <optional>

class FilterRegistry {
public:
    static constexpr std::size_t max_filters = consolidator::eq::max_filter_slots;

    void clear() {
        contracts_.fill(std::nullopt);
    }

    bool empty() const {
        for (const auto& contract : contracts_) {
            if (contract) {
                return false;
            }
        }

        return true;
    }

    void define(const FilterContract& contract) {
        if (contract.slot < 0) {
            return;
        }

        const auto slot = static_cast<std::size_t>(contract.slot);
        if (slot >= max_filters) {
            return;
        }

        contracts_[slot] = contract;
    }

    void undefine(std::size_t slot) {
        if (slot >= max_filters) {
            return;
        }

        contracts_[slot].reset();
    }

    const std::optional<FilterContract>& at(std::size_t slot) const {
        return contracts_[slot];
    }

    std::optional<FilterContract>& at(std::size_t slot) {
        return contracts_[slot];
    }

    const std::array<std::optional<FilterContract>, max_filters>& all() const {
        return contracts_;
    }

private:
    std::array<std::optional<FilterContract>, max_filters> contracts_{};
};
