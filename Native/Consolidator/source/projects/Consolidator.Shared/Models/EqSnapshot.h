#pragma once

#include "FilterState.h"

#include <algorithm>
#include <string>
#include <vector>

namespace consolidator::models {

struct EqBank {
    long bankId = 0;
    std::string name;
    bool bypass = false;
    bool solo = false;
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
    long selectedBankId = 0;
    std::vector<EqBank> banks;

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

    bool HasSoloBanks() const {
        return std::any_of(banks.begin(), banks.end(), [](const EqBank& bank) {
            return bank.solo;
        });
    }

    bool IsBypassed(const EqBank& bank) const {
        return bank.bypass || (HasSoloBanks() && !bank.solo);
    }
};

} // namespace consolidator::models
