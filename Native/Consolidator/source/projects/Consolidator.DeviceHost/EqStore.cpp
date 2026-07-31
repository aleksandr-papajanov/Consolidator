#include "EqStore.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace consolidator::host {

namespace {

std::optional<long> ToId(std::int64_t value) {
    if (value < models::EqSnapshot::SystemBankId || value > models::EqSnapshot::LastUserBankId) {
        return std::nullopt;
    }
    return static_cast<long>(value);
}

bool IsEditableLinkId(const std::string& value) {
    if (value.size() < 7 || value.size() > 8 || value.compare(0, 6, "group.") != 0) {
        return false;
    }
    const auto suffix = value.substr(6);
    if (suffix.empty() || !std::all_of(suffix.begin(), suffix.end(),
        [](char character) { return character >= '0' && character <= '9'; })) {
        return false;
    }
    const auto groupId = std::stol(suffix);
    return groupId >= 1 && groupId <= 10;
}

} // namespace

EqStore::EqStore(CommitHandler commitHandler)
    : definitions(domain::FilterDefinitions()), commitHandler(std::move(commitHandler)) {
    for (long bankId = models::EqSnapshot::SystemBankId;
         bankId <= models::EqSnapshot::LastUserBankId;
         ++bankId) {
        state.banks.push_back(CreateDefaultBank(bankId));
    }
}

const domain::EqState& EqStore::State() const noexcept { return state; }
domain::StoreRevision EqStore::Revision() const noexcept { return revision; }

models::EqBank EqStore::CreateDefaultBank(long bankId) const {
    models::EqBank bank;
    bank.bankId = bankId;
    if (bankId == models::EqSnapshot::SystemBankId) return bank;
    if (bankId == models::EqSnapshot::GlobalBankId) {
        bank.linkId = models::EqSnapshot::GlobalLinkId;
    }
    for (const auto& [filterId, definition] : definitions) {
        bank.filters.push_back({ filterId, definition.DefaultValues(), definition.defaultBypass });
    }
    return bank;
}

models::FilterState* EqStore::FindFilter(domain::BankId bankId, domain::FilterId filterId) {
    const auto bankValue = ToId(bankId.value);
    if (!bankValue || filterId.value < 1 || filterId.value > std::numeric_limits<long>::max()) return nullptr;
    auto* bank = state.FindBank(*bankValue);
    return bank ? bank->FindFilter(static_cast<long>(filterId.value)) : nullptr;
}

const models::FilterDefinition* EqStore::FindDefinition(domain::FilterId filterId) const {
    if (filterId.value < 1 || filterId.value > std::numeric_limits<long>::max()) return nullptr;
    const auto definition = definitions.find(static_cast<long>(filterId.value));
    return definition == definitions.end() ? nullptr : &definition->second;
}

UpdateResult EqStore::SetParameter(const domain::SetEqParameterCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_user_bank");
    auto* filter = FindFilter(command.bankId, command.filterId);
    const auto* definition = FindDefinition(command.filterId);
    if (!filter || !definition) return Reject("invalid_filter");
    const auto parameterIndex = definition->ParameterIndex(command.parameter);
    const auto* parameter = definition->FindParameter(command.parameter);
    if (!parameterIndex || !parameter || !std::isfinite(command.value) ||
        *parameterIndex >= filter->values.size()) {
        return Reject("invalid_parameter");
    }
    if (filter->values[*parameterIndex] == command.value) return { UpdateStatus::Unchanged, {} };
    filter->values[*parameterIndex] = command.value;
    return Commit(command.requestId);
}

UpdateResult EqStore::SetParameterAtIndex(const domain::SetEqParameterIndexCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_user_bank");
    auto* filter = FindFilter(command.bankId, command.filterId);
    const auto* definition = FindDefinition(command.filterId);
    if (!filter || !definition || command.parameterIndex >= definition->parameters.size() ||
        command.parameterIndex >= filter->values.size()) {
        return Reject("invalid_parameter_index");
    }
    if (!std::isfinite(command.value)) {
        return Reject("invalid_parameter");
    }
    if (filter->values[command.parameterIndex] == command.value) return { UpdateStatus::Unchanged, {} };
    filter->values[command.parameterIndex] = command.value;
    return Commit(command.requestId);
}

UpdateResult EqStore::SetBypass(const domain::SetEqBypassCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_user_bank");
    auto* filter = FindFilter(command.bankId, command.filterId);
    if (!filter) return Reject("invalid_filter");
    if (filter->bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    filter->bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult EqStore::ResetFilter(const domain::ResetEqFilterCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_user_bank");
    auto* filter = FindFilter(command.bankId, command.filterId);
    const auto* definition = FindDefinition(command.filterId);
    if (!filter || !definition) return Reject("invalid_filter");
    const auto values = definition->DefaultValues();
    if (filter->values == values && filter->bypass == definition->defaultBypass) return { UpdateStatus::Unchanged, {} };
    filter->values = values;
    filter->bypass = definition->defaultBypass;
    return Commit(command.requestId);
}

UpdateResult EqStore::SetChainBypass(const domain::SetEqChainBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult EqStore::SetChainSolo(const domain::SetEqChainSoloCommand& command) {
    if (state.solo == command.solo) return { UpdateStatus::Unchanged, {} };
    state.solo = command.solo;
    return Commit(command.requestId);
}

UpdateResult EqStore::ResetChain(const domain::ResetEqChainCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_user_bank");
    auto* bank = state.FindBank(*bankId);
    if (!bank) return Reject("invalid_user_bank");
    const auto defaults = CreateDefaultBank(*bankId);
    if (bank->filters == defaults.filters) return { UpdateStatus::Unchanged, {} };
    bank->filters = defaults.filters;
    return Commit(command.requestId);
}

UpdateResult EqStore::ResetAll(const domain::ResetAllEqBanksCommand& command) {
    bool changed = false;
    for (auto& bank : state.banks) {
        const auto defaults = CreateDefaultBank(bank.bankId);
        if (bank.filters == defaults.filters) continue;
        bank.filters = defaults.filters;
        changed = true;
    }
    if (!changed) return { UpdateStatus::Unchanged, {} };
    return Commit(command.requestId);
}

UpdateResult EqStore::SelectBank(const domain::SelectEqBankCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_user_bank");
    if (state.selectedBankId == *bankId) return { UpdateStatus::Unchanged, {} };
    state.selectedBankId = *bankId;
    return Commit(command.requestId);
}

UpdateResult EqStore::JoinBanks(const domain::JoinEqBanksCommand& command) {
    if (command.bankIds.empty()) return Reject("invalid_join_banks");
    std::set<long> bankIds;
    for (const auto bankId : command.bankIds) {
        const auto value = ToId(bankId.value);
        if (!value || !models::EqSnapshot::IsUserBankId(*value) || !bankIds.insert(*value).second) {
            return Reject("invalid_join_banks");
        }
    }

    auto* systemBank = state.FindBank(models::EqSnapshot::SystemBankId);
    if (!systemBank) return Reject("missing_system_bank");
    std::vector<models::FilterState> accumulatedFilters;
    for (const auto bankId : bankIds) {
        const auto* bank = state.FindBank(bankId);
        if (!bank) return Reject("invalid_join_banks");
        for (const auto& filter : bank->filters) {
            const auto definition = definitions.find(filter.filterId);
            if (definition != definitions.end() && !filter.bypass &&
                std::abs(definition->second.Value(filter.values, "gain", 0.0)) > 1.0e-12) {
                accumulatedFilters.push_back(filter);
            }
        }
    }
    if (accumulatedFilters.empty()) return { UpdateStatus::Unchanged, {} };

    systemBank->filters.insert(systemBank->filters.end(), accumulatedFilters.begin(), accumulatedFilters.end());
    for (const auto bankId : bankIds) {
        auto* bank = state.FindBank(bankId);
        bank->filters = CreateDefaultBank(bankId).filters;
    }
    return Commit(command.requestId);
}

UpdateResult EqStore::SetBankLink(const domain::SetEqBankLinkCommand& command) {
    const auto bankId = ToId(command.bankId.value);
    if (!bankId || *bankId < models::EqSnapshot::FirstLinkableBankId ||
        *bankId > models::EqSnapshot::LastLinkableBankId ||
        (!command.linkId.empty() && !IsEditableLinkId(command.linkId))) {
        return Reject("invalid_linkable_bank");
    }
    auto* bank = state.FindBank(*bankId);
    if (!bank) return Reject("invalid_user_bank");
    if (!command.linkId.empty()) {
        for (const auto& candidate : state.banks) {
            if (candidate.bankId != *bankId && candidate.linkId == command.linkId) {
                return Reject("duplicate_local_link_member");
            }
        }
    }
    if (bank->linkId == command.linkId) return { UpdateStatus::Unchanged, {} };
    bank->linkId = command.linkId;
    return Commit(command.requestId);
}

bool EqStore::IsUserBankEmpty(long bankId) const {
    const auto* bank = state.FindBank(bankId);
    if (!bank || !models::EqSnapshot::IsUserBankId(bankId)) return false;
    return bank->filters == CreateDefaultBank(bankId).filters;
}

UpdateResult EqStore::ApplyFitResult(const domain::CompleteFitCommand& command) {
    const auto bankId = ToId(command.result.bankId.value);
    if (!bankId || !models::EqSnapshot::IsUserBankId(*bankId)) return Reject("invalid_fit_result");
    auto* bank = state.FindBank(*bankId);
    if (!bank) return Reject("invalid_fit_result");
    const auto previousFilters = bank->filters;
    if (!ApplyFitFilters(*bank, command.result)) return Reject("invalid_fit_result");
    if (previousFilters == bank->filters) return { UpdateStatus::Unchanged, {} };
    return Commit(command.requestId);
}

UpdateResult EqStore::ApplyCommitHiddenResult(const domain::CompleteFitCommand& command) {
    const auto bankId = ToId(command.result.bankId.value);
    auto* systemBank = state.FindBank(models::EqSnapshot::SystemBankId);
    auto* targetBank = bankId ? state.FindBank(*bankId) : nullptr;
    if (!bankId || *bankId != models::EqSnapshot::IndividualBankId || !systemBank || !targetBank ||
        systemBank->filters.empty() || !IsUserBankEmpty(*bankId)) {
        return Reject("invalid_commit_hidden");
    }
    if (!ApplyFitFilters(*targetBank, command.result)) return Reject("invalid_fit_result");
    systemBank->filters.clear();
    return Commit(command.requestId);
}

UpdateResult EqStore::ApplyCommitAllResult(
    const domain::CompleteFitCommand& command,
    domain::StoreRevision expectedRevision
) {
    if (revision != expectedRevision) return Reject("stale_commit_state");
    const auto bankId = ToId(command.result.bankId.value);
    if (!bankId || *bankId != models::EqSnapshot::IndividualBankId) {
        return Reject("invalid_commit_all");
    }

    auto nextState = state;
    auto* targetBank = nextState.FindBank(*bankId);
    auto* systemBank = nextState.FindBank(models::EqSnapshot::SystemBankId);
    if (!targetBank || !systemBank || !ApplyFitFilters(*targetBank, command.result)) {
        return Reject("invalid_fit_result");
    }

    for (long userBankId = models::EqSnapshot::FirstUserBankId;
         userBankId <= models::EqSnapshot::LastUserBankId;
         ++userBankId) {
        auto* bank = nextState.FindBank(userBankId);
        if (!bank) return Reject("invalid_commit_all");
        if (userBankId == *bankId) continue;
        bank->filters = CreateDefaultBank(userBankId).filters;
    }
    systemBank->filters.clear();

    state = std::move(nextState);
    return Commit(command.requestId);
}

bool EqStore::ApplyFitFilters(models::EqBank& bank, const domain::FitResult& result) const {
    if (result.filters.size() != definitions.size()) return false;
    auto nextFilters = bank.filters;
    std::set<long> seenFilterIds;
    for (const auto& candidate : result.filters) {
        if (!seenFilterIds.insert(candidate.filterId).second) return false;
        const auto* definition = FindDefinition({ candidate.filterId });
        const auto destination = std::find_if(nextFilters.begin(), nextFilters.end(),
            [filterId = candidate.filterId](const auto& filter) { return filter.filterId == filterId; });
        if (!definition || destination == nextFilters.end() || candidate.values.size() != definition->parameters.size()) return false;
        for (std::size_t index = 0; index < candidate.values.size(); ++index) {
            const auto value = candidate.values[index];
            if (!std::isfinite(value)) return false;
        }
        destination->values = candidate.values;
        destination->bypass = candidate.bypass;
    }
    bank.filters = std::move(nextFilters);
    return true;
}

UpdateResult EqStore::Replace(domain::EqState nextState, domain::StoreRevision nextRevision) {
    if (!models::EqSnapshot::IsUserBankId(nextState.selectedBankId) ||
        nextState.banks.size() != static_cast<std::size_t>(models::EqSnapshot::BankCount)) {
        return Reject("invalid_persisted_eq_state");
    }
    for (long bankId = models::EqSnapshot::SystemBankId;
         bankId <= models::EqSnapshot::LastUserBankId;
         ++bankId) {
        auto* bank = nextState.FindBank(bankId);
        if (!bank || bank->bankId != bankId ||
            (bankId == models::EqSnapshot::SystemBankId && !bank->linkId.empty()) ||
            (bankId == models::EqSnapshot::IndividualBankId && !bank->linkId.empty()) ||
            (bankId == models::EqSnapshot::GlobalBankId && bank->linkId != models::EqSnapshot::GlobalLinkId) ||
            (bankId >= models::EqSnapshot::FirstLinkableBankId &&
                bankId <= models::EqSnapshot::LastLinkableBankId &&
                !bank->linkId.empty() && !IsEditableLinkId(bank->linkId)) ||
            (models::EqSnapshot::IsUserBankId(bankId) && bank->filters.size() != definitions.size())) {
            return Reject("invalid_persisted_eq_state");
        }
        auto expectedDefinition = definitions.begin();
        for (const auto& filter : bank->filters) {
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end() ||
                (models::EqSnapshot::IsUserBankId(bankId) &&
                    (expectedDefinition == definitions.end() || filter.filterId != expectedDefinition->first)) ||
                filter.values.size() != definition->second.parameters.size()) {
                return Reject("invalid_persisted_eq_state");
            }
            for (std::size_t index = 0; index < filter.values.size(); ++index) {
                const auto value = filter.values[index];
                if (!std::isfinite(value)) {
                    return Reject("invalid_persisted_eq_state");
                }
            }
            if (models::EqSnapshot::IsUserBankId(bankId)) ++expectedDefinition;
        }
    }
    state = std::move(nextState);
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

UpdateResult EqStore::Commit(domain::RequestId requestId) {
    ++revision;
    if (commitHandler) commitHandler(revision, requestId);
    return { UpdateStatus::Changed, {} };
}

UpdateResult EqStore::Reject(const char* error) const { return { UpdateStatus::Rejected, error }; }

} // namespace consolidator::host
