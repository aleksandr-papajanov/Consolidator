#pragma once

#include "FilterState.h"

#include <algorithm>
#include <string>
#include <vector>

namespace consolidator::models {

struct EqBank {
    long bankId = 0;
    std::string linkId;
    std::vector<FilterState> filters;

    FilterState* FindFilter(long filterId) {
        const auto filter = std::find_if(filters.begin(), filters.end(),
            [filterId](const FilterState& candidate) {
                return candidate.filterId == filterId;
            });
        return filter == filters.end() ? nullptr : &*filter;
    }

    const FilterState* FindFilter(long filterId) const {
        const auto filter = std::find_if(filters.begin(), filters.end(),
            [filterId](const FilterState& candidate) {
                return candidate.filterId == filterId;
            });
        return filter == filters.end() ? nullptr : &*filter;
    }
};

struct EqSnapshot {
    static constexpr long SystemBankId = 0;
    static constexpr long FirstUserBankId = 1;
    static constexpr long LastUserBankId = 6;
    static constexpr long BankCount = LastUserBankId + 1;

    long selectedBankId = FirstUserBankId;
    bool bypass = false;
    bool solo = false;
    std::vector<EqBank> banks;

    static bool IsUserBankId(long bankId) noexcept {
        return bankId >= FirstUserBankId && bankId <= LastUserBankId;
    }

    static bool IsKnownBankId(long bankId) noexcept {
        return bankId >= SystemBankId && bankId <= LastUserBankId;
    }

    EqBank* FindBank(long bankId) {
        const auto bank = std::find_if(banks.begin(), banks.end(),
            [bankId](const EqBank& candidate) {
                return candidate.bankId == bankId;
            });
        return bank == banks.end() ? nullptr : &*bank;
    }

    const EqBank* FindBank(long bankId) const {
        const auto bank = std::find_if(banks.begin(), banks.end(),
            [bankId](const EqBank& candidate) {
                return candidate.bankId == bankId;
            });
        return bank == banks.end() ? nullptr : &*bank;
    }

    EqBank* SelectedBank() {
        return FindBank(selectedBankId);
    }

    const EqBank* SelectedBank() const {
        return FindBank(selectedBankId);
    }

    bool IsBypassed() const noexcept {
        return bypass;
    }
};

} // namespace consolidator::models
