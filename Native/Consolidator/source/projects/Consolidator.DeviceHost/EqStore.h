#pragma once

#include "Definitions/Definitions.h"
#include "UpdateResult.h"

#include "Commands/Commands.h"

#include <functional>
#include <vector>

namespace consolidator::host {

class EqStore final {
public:
    using CommitHandler = std::function<void(domain::StoreRevision, domain::RequestId)>;

    explicit EqStore(CommitHandler commitHandler = {});

    const domain::EqState& State() const noexcept;
    domain::StoreRevision Revision() const noexcept;
    bool IsUserBankEmpty(long bankId) const;

    UpdateResult SetParameter(const domain::SetEqParameterCommand& command);
    UpdateResult SetParameterAtIndex(const domain::SetEqParameterIndexCommand& command);
    UpdateResult SetBypass(const domain::SetEqBypassCommand& command);
    UpdateResult ResetFilter(const domain::ResetEqFilterCommand& command);
    UpdateResult SetChainBypass(const domain::SetEqChainBypassCommand& command);
    UpdateResult SetChainSolo(const domain::SetEqChainSoloCommand& command);
    UpdateResult ResetChain(const domain::ResetEqChainCommand& command);
    UpdateResult JoinBanks(const domain::JoinEqBanksCommand& command);
    UpdateResult SetBankLink(const domain::SetEqBankLinkCommand& command);
    UpdateResult SelectBank(const domain::SelectEqBankCommand& command);
    UpdateResult ApplyFitResult(const domain::CompleteFitCommand& command);
    UpdateResult ApplyCommitHiddenResult(const domain::CompleteFitCommand& command);

    UpdateResult Replace(domain::EqState state, domain::StoreRevision revision);

private:
    models::EqBank CreateDefaultBank(long bankId) const;
    models::FilterState* FindFilter(domain::BankId bankId, domain::FilterId filterId);
    const models::FilterDefinition* FindDefinition(domain::FilterId filterId) const;
    bool ApplyFitFilters(models::EqBank& bank, const domain::FitResult& result) const;
    UpdateResult Commit(domain::RequestId requestId);
    UpdateResult Reject(const char* error) const;

    domain::EqState state;
    domain::StoreRevision revision = 0;
    domain::FilterDefinitionCatalog definitions;
    CommitHandler commitHandler;
};

} // namespace consolidator::host
