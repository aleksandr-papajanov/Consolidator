#pragma once

#include "c74_min.h"
#include "Models/EqSnapshot.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace consolidator::maxadapter {

class MaxEqSnapshotAdapter final {
public:
    static std::optional<models::EqSnapshot> Read(
        const std::string& dictionaryName,
        long selectedBankId
    ) {
        try {
            c74::min::dict snapshot{ c74::min::symbol{ dictionaryName.c_str() } };
            c74::min::dict banks{ static_cast<c74::min::atom>(snapshot.at("banks")) };
            models::EqSnapshot result;
            result.selectedBankId = selectedBankId;
            for (const auto& bankKey : banks.keys()) {
                models::EqBank bank;
                bank.bankId = std::stol(static_cast<const char* const>(bankKey));
                c74::min::dict sourceBank{ static_cast<c74::min::atom>(banks.at(bankKey)) };
                try {
                    bank.name = static_cast<std::string>(static_cast<c74::min::atom>(sourceBank.at("name")));
                }
                catch (...) {}
                c74::min::dict filters{ static_cast<c74::min::atom>(sourceBank.at("filters")) };
                for (const auto& filterKey : filters.keys()) {
                    models::FilterState filter;
                    filter.filterId = std::stol(static_cast<const char* const>(filterKey));
                    filter.bankIndex = bank.bankId;
                    c74::min::dict sourceFilter{ static_cast<c74::min::atom>(filters.at(filterKey)) };
                    const auto values = static_cast<std::vector<c74::min::number>>(sourceFilter.at("values"));
                    filter.values.assign(values.begin(), values.end());
                    filter.bypass = static_cast<double>(
                        static_cast<c74::min::atom>(sourceFilter.at("bypass"))) != 0.0;
                    bank.filters.push_back(std::move(filter));
                }
                std::sort(bank.filters.begin(), bank.filters.end(),
                    [](const auto& left, const auto& right) {
                        return left.filterId < right.filterId;
                    });
                result.banks.push_back(std::move(bank));
            }
            std::sort(result.banks.begin(), result.banks.end(),
                [](const auto& left, const auto& right) {
                    return left.bankId < right.bankId;
                });
            return result;
        }
        catch (...) {
            return std::nullopt;
        }
    }
};

} // namespace consolidator::maxadapter
