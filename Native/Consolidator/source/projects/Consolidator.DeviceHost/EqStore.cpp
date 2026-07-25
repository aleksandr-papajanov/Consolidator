#include "EqStore.h"
#include "Definitions/BankNameGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <utility>

namespace consolidator::host {

namespace {

std::optional<long> ToLegacyId(std::int64_t value) {
    if (value < 1 || value > std::numeric_limits<long>::max()) return std::nullopt;
    return static_cast<long>(value);
}

} // namespace

EqStore::EqStore(CommitHandler commitHandler)
    : definitions(domain::FilterDefinitions()), commitHandler(std::move(commitHandler)) {
    state.selectedBankId = 1;
    state.banks.push_back({ 1, domain::BankNameGenerator::Generate(1), false, false, {} });
    auto& bank = state.banks.back();
    for (const auto& [filterId, definition] : definitions) {
        bank.filters.push_back({ filterId, definition.DefaultValues(), definition.defaultBypass });
    }
}

const domain::EqState& EqStore::State() const noexcept {
    return state;
}

domain::StoreRevision EqStore::Revision() const noexcept {
    return revision;
}

models::FilterState* EqStore::FindFilter(domain::BankId bankId, domain::FilterId filterId) {
    const auto bankValue = ToLegacyId(bankId.value);
    const auto filterValue = ToLegacyId(filterId.value);
    if (!bankValue || !filterValue) return nullptr;
    auto* bank = state.FindBank(*bankValue);
    return bank ? bank->FindFilter(*filterValue) : nullptr;
}

const models::FilterDefinition* EqStore::FindDefinition(domain::FilterId filterId) const {
    const auto filterValue = ToLegacyId(filterId.value);
    if (!filterValue) return nullptr;
    const auto definition = definitions.find(*filterValue);
    return definition == definitions.end() ? nullptr : &definition->second;
}

UpdateResult EqStore::SetParameter(const domain::SetEqParameterCommand& command) {
    auto* filter = FindFilter(command.bankId, command.filterId);
    const auto* definition = FindDefinition(command.filterId);
    if (!filter || !definition) return Reject("invalid_filter");
    const auto parameterIndex = definition->ParameterIndex(command.parameter);
    const auto* parameter = definition->FindParameter(command.parameter);
    if (!parameterIndex || !parameter || !std::isfinite(command.value)) return Reject("invalid_parameter");
    if (command.value < parameter->range.minimum || command.value > parameter->range.maximum) {
        return Reject("parameter_out_of_range");
    }
    if (*parameterIndex >= filter->values.size()) return Reject("invalid_filter_state");
    if (filter->values[*parameterIndex] == command.value) return { UpdateStatus::Unchanged, {} };
    filter->values[*parameterIndex] = command.value;
    return Commit(command.requestId);
}

UpdateResult EqStore::SetBypass(const domain::SetEqBypassCommand& command) {
    auto* filter = FindFilter(command.bankId, command.filterId);
    if (!filter) return Reject("invalid_filter");
    if (filter->bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    filter->bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult EqStore::ResetFilter(const domain::ResetEqFilterCommand& command) {
    auto* filter = FindFilter(command.bankId, command.filterId);
    const auto* definition = FindDefinition(command.filterId);
    if (!filter || !definition) return Reject("invalid_filter");
    const auto values = definition->DefaultValues();
    if (filter->values == values && filter->bypass == definition->defaultBypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    filter->values = values;
    filter->bypass = definition->defaultBypass;
    return Commit(command.requestId);
}

UpdateResult EqStore::SetChainBypass(const domain::SetEqChainBypassCommand& command) {
    const auto bankId = ToLegacyId(command.bankId.value);
    auto* bank = bankId ? state.FindBank(*bankId) : nullptr;
    if (!bank) return Reject("invalid_bank");
    if (bank->bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    bank->bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult EqStore::ResetChain(const domain::ResetEqChainCommand& command) {
    const auto bankId = ToLegacyId(command.bankId.value);
    auto* bank = bankId ? state.FindBank(*bankId) : nullptr;
    if (!bank) return Reject("invalid_bank");

    auto changed = bank->bypass;
    bank->bypass = false;
    for (auto& filter : bank->filters) {
        const auto definition = definitions.find(filter.filterId);
        if (definition == definitions.end()) return Reject("invalid_filter");
        const auto values = definition->second.DefaultValues();
        changed = changed || filter.values != values ||
            filter.bypass != definition->second.defaultBypass;
        filter.values = values;
        filter.bypass = definition->second.defaultBypass;
    }
    return changed ? Commit(command.requestId) : UpdateResult{ UpdateStatus::Unchanged, {} };
}

UpdateResult EqStore::AddBank(const domain::AddEqBankCommand& command) {
    const auto nextId = nextBankId++;
    const auto name = command.name.empty() ? domain::BankNameGenerator::Generate(nextId) : command.name;
    state.banks.push_back({ nextId, name, false, false, {} });
    auto& bank = state.banks.back();
    for (const auto& [filterId, definition] : definitions) {
        bank.filters.push_back({ filterId, definition.DefaultValues(), definition.defaultBypass });
    }
    state.selectedBankId = nextId;
    return Commit(command.requestId);
}

UpdateResult EqStore::RemoveBank(const domain::RemoveEqBankCommand& command) {
    return RemoveBanks({ command.requestId, { command.bankId } });
}

UpdateResult EqStore::RemoveBanks(const domain::RemoveEqBanksCommand& command) {
    if (command.bankIds.empty() || !HasBanks(command.bankIds)) return Reject("invalid_bank");

    std::set<long> removedIds;
    for (const auto bankId : command.bankIds) {
        const auto id = ToLegacyId(bankId.value);
        if (!id || !removedIds.insert(*id).second) return Reject("invalid_bank");
    }
    if (removedIds.size() >= state.banks.size()) return Reject("cannot_remove_last_bank");

    const auto selectedRemoved = removedIds.contains(state.selectedBankId);
    const auto selectedIndex = std::find_if(state.banks.begin(), state.banks.end(),
        [id = state.selectedBankId](const auto& bank) { return bank.bankId == id; });
    const auto fallbackIndex = static_cast<std::size_t>(std::distance(state.banks.begin(), selectedIndex));
    std::erase_if(state.banks, [&removedIds](const auto& bank) {
        return removedIds.contains(bank.bankId);
    });
    if (selectedRemoved) {
        state.selectedBankId = state.banks[std::min(fallbackIndex, state.banks.size() - 1)].bankId;
    }
    return Commit(command.requestId);
}

UpdateResult EqStore::SetBanksBypass(const domain::SetEqBanksBypassCommand& command) {
    if (command.bankIds.empty() || !HasBanks(command.bankIds)) return Reject("invalid_bank");
    auto changed = false;
    std::set<long> seenIds;
    for (const auto bankId : command.bankIds) {
        const auto id = ToLegacyId(bankId.value);
        if (!id || !seenIds.insert(*id).second) return Reject("invalid_bank");
        auto* bank = state.FindBank(*id);
        changed = changed || bank->bypass != command.bypass;
        bank->bypass = command.bypass;
    }
    return changed ? Commit(command.requestId) : UpdateResult{ UpdateStatus::Unchanged, {} };
}

UpdateResult EqStore::SoloBanks(const domain::SoloEqBanksCommand& command) {
    if (command.bankIds.empty() || !HasBanks(command.bankIds)) return Reject("invalid_bank");
    std::set<long> soloIds;
    for (const auto bankId : command.bankIds) {
        const auto id = ToLegacyId(bankId.value);
        if (!id || !soloIds.insert(*id).second) return Reject("invalid_bank");
    }
    const auto alreadySoloed = std::all_of(command.bankIds.begin(), command.bankIds.end(),
        [this](const auto bankId) {
            const auto id = ToLegacyId(bankId.value);
            const auto* bank = id ? state.FindBank(*id) : nullptr;
            return bank && bank->solo;
        });
    auto changed = false;
    for (auto& bank : state.banks) {
        const auto nextSolo = alreadySoloed ? false : soloIds.contains(bank.bankId);
        changed = changed || bank.solo != nextSolo;
        bank.solo = nextSolo;
    }
    return changed ? Commit(command.requestId) : UpdateResult{ UpdateStatus::Unchanged, {} };
}

UpdateResult EqStore::RenameBank(const domain::RenameEqBankCommand& command) {
    const auto bankId = ToLegacyId(command.bankId.value);
    auto* bank = bankId ? state.FindBank(*bankId) : nullptr;
    if (!bank || command.name.empty()) return Reject("invalid_bank");
    if (bank->name == command.name) return { UpdateStatus::Unchanged, {} };
    bank->name = command.name;
    return Commit(command.requestId);
}

UpdateResult EqStore::SelectBank(const domain::SelectEqBankCommand& command) {
    const auto bankId = ToLegacyId(command.bankId.value);
    if (!bankId || !state.FindBank(*bankId)) return Reject("invalid_bank");
    if (state.selectedBankId == *bankId) return { UpdateStatus::Unchanged, {} };
    state.selectedBankId = *bankId;
    return Commit(command.requestId);
}

UpdateResult EqStore::ApplyFitResult(const domain::CompleteFitCommand& command) {
    const auto bankId = ToLegacyId(command.result.bankId.value);
    auto* bank = bankId ? state.FindBank(*bankId) : nullptr;
    if (!bank) return Reject("invalid_fit_result");
    const auto previousFilters = bank->filters;
    if (!ApplyFitFilters(*bank, command.result)) return Reject("invalid_fit_result");
    const auto unchanged = std::equal(previousFilters.begin(), previousFilters.end(), bank->filters.begin(),
        [](const auto& left, const auto& right) {
            return left.filterId == right.filterId && left.values == right.values && left.bypass == right.bypass;
        });
    if (unchanged) return { UpdateStatus::Unchanged, {} };
    return Commit(command.requestId);
}

UpdateResult EqStore::ApplyJoinFitResult(
    const domain::CompleteFitCommand& command,
    const std::vector<domain::BankId>& joinedBankIds
) {
    if (joinedBankIds.empty() || !HasBanks(joinedBankIds)) return Reject("invalid_join_banks");
    const auto targetId = ToLegacyId(command.result.bankId.value);
    if (!targetId || std::none_of(joinedBankIds.begin(), joinedBankIds.end(),
        [targetId](const auto bankId) { return bankId.value == *targetId; })) {
        return Reject("invalid_join_banks");
    }
    auto* targetBank = state.FindBank(*targetId);
    if (!targetBank || !ApplyFitFilters(*targetBank, command.result)) return Reject("invalid_fit_result");

    std::set<long> joinedIds;
    for (const auto bankId : joinedBankIds) {
        const auto id = ToLegacyId(bankId.value);
        if (!id || !joinedIds.insert(*id).second) return Reject("invalid_join_banks");
    }
    const auto name = domain::BankNameGenerator::Generate(nextBankId);
    models::EqBank joinedBank{
        nextBankId++, name, targetBank->bypass, targetBank->solo, targetBank->filters
    };
    std::erase_if(state.banks, [&joinedIds](const auto& bank) {
        return joinedIds.contains(bank.bankId);
    });
    state.banks.push_back(std::move(joinedBank));
    state.selectedBankId = state.banks.back().bankId;
    return Commit(command.requestId);
}

bool EqStore::HasBanks(const std::vector<domain::BankId>& bankIds) const {
    return std::all_of(bankIds.begin(), bankIds.end(), [this](const auto bankId) {
        const auto id = ToLegacyId(bankId.value);
        return id && state.FindBank(*id);
    });
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
        if (!definition || destination == nextFilters.end() ||
            candidate.values.size() != definition->parameters.size()) {
            return false;
        }
        for (std::size_t index = 0; index < candidate.values.size(); ++index) {
            const auto value = candidate.values[index];
            const auto& range = definition->parameters[index].range;
            if (!std::isfinite(value) || value < range.minimum || value > range.maximum) {
                return false;
            }
        }
        destination->values = candidate.values;
        destination->bypass = candidate.bypass;
    }
    bank.filters = std::move(nextFilters);
    return true;
}

UpdateResult EqStore::Replace(domain::EqState nextState, domain::StoreRevision nextRevision) {
    if (nextState.banks.empty() || !nextState.FindBank(nextState.selectedBankId)) {
        return Reject("invalid_persisted_eq_state");
    }
    long previousBankId = 0;
    for (auto& bank : nextState.banks) {
        if (bank.bankId <= previousBankId || bank.name.empty() ||
            bank.filters.size() != definitions.size()) {
            return Reject("invalid_persisted_eq_state");
        }
        previousBankId = bank.bankId;
        auto definition = definitions.begin();
        for (auto& filter : bank.filters) {
            if (definition == definitions.end() || filter.filterId != definition->first ||
                filter.values.size() != definition->second.parameters.size()) {
                return Reject("invalid_persisted_eq_state");
            }
            for (std::size_t index = 0; index < filter.values.size(); ++index) {
                const auto value = filter.values[index];
                const auto& range = definition->second.parameters[index].range;
                if (!std::isfinite(value) || value < range.minimum || value > range.maximum) {
                    return Reject("invalid_persisted_eq_state");
                }
            }
            ++definition;
        }
    }
    state = std::move(nextState);
    revision = nextRevision;
    nextBankId = 1;
    for (const auto& bank : state.banks) nextBankId = std::max(nextBankId, bank.bankId + 1);
    return { UpdateStatus::Changed, {} };
}

UpdateResult EqStore::Commit(domain::RequestId requestId) {
    ++revision;
    if (commitHandler) commitHandler(revision, requestId);
    return { UpdateStatus::Changed, {} };
}

UpdateResult EqStore::Reject(const char* error) const {
    return { UpdateStatus::Rejected, error };
}

} // namespace consolidator::host
