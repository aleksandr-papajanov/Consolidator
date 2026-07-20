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
    state.banks.push_back({ 1, domain::BankNameGenerator::Generate(1), {} });
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

UpdateResult EqStore::AddBank(const domain::AddEqBankCommand& command) {
    const auto nextId = nextBankId++;
    const auto name = command.name.empty() ? domain::BankNameGenerator::Generate(nextId) : command.name;
    state.banks.push_back({ nextId, name, {} });
    auto& bank = state.banks.back();
    for (const auto& [filterId, definition] : definitions) {
        bank.filters.push_back({ filterId, definition.DefaultValues(), definition.defaultBypass });
    }
    state.selectedBankId = nextId;
    return Commit(command.requestId);
}

UpdateResult EqStore::RemoveBank(const domain::RemoveEqBankCommand& command) {
    if (state.banks.size() <= 1) return Reject("cannot_remove_last_bank");
    const auto bankId = ToLegacyId(command.bankId.value);
    if (!bankId) return Reject("invalid_bank");
    const auto item = std::find_if(state.banks.begin(), state.banks.end(),
        [id = *bankId](const auto& bank) { return bank.bankId == id; });
    if (item == state.banks.end()) return Reject("invalid_bank");
    const auto removedIndex = static_cast<std::size_t>(std::distance(state.banks.begin(), item));
    state.banks.erase(item);
    if (state.selectedBankId == *bankId) {
        const auto nextIndex = std::min(removedIndex, state.banks.size() - 1);
        state.selectedBankId = state.banks[nextIndex].bankId;
    }
    return Commit(command.requestId);
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
    if (!bank || command.result.filters.size() != definitions.size()) return Reject("invalid_fit_result");

    auto nextFilters = bank->filters;
    std::set<long> seenFilterIds;
    for (const auto& candidate : command.result.filters) {
        if (!seenFilterIds.insert(candidate.filterId).second) return Reject("invalid_fit_result");
        const auto* definition = FindDefinition({ candidate.filterId });
        const auto destination = std::find_if(nextFilters.begin(), nextFilters.end(),
            [filterId = candidate.filterId](const auto& filter) { return filter.filterId == filterId; });
        if (!definition || destination == nextFilters.end() ||
            candidate.values.size() != definition->parameters.size()) {
            return Reject("invalid_fit_result");
        }
        for (std::size_t index = 0; index < candidate.values.size(); ++index) {
            const auto value = candidate.values[index];
            const auto& range = definition->parameters[index].range;
            if (!std::isfinite(value) || value < range.minimum || value > range.maximum) {
                return Reject("invalid_fit_result");
            }
        }
        destination->values = candidate.values;
        destination->bypass = candidate.bypass;
    }
    const auto unchanged = std::equal(bank->filters.begin(), bank->filters.end(), nextFilters.begin(), nextFilters.end(),
        [](const auto& left, const auto& right) {
            return left.filterId == right.filterId && left.values == right.values && left.bypass == right.bypass;
        });
    if (unchanged) return { UpdateStatus::Unchanged, {} };
    bank->filters = std::move(nextFilters);
    return Commit(command.requestId);
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
