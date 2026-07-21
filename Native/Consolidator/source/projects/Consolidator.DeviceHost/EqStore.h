#pragma once

#include "Definitions/Definitions.h"
#include "UpdateResult.h"

#include "Commands/Commands.h"

#include <functional>

namespace consolidator::host {

class EqStore final {
public:
    using CommitHandler = std::function<void(domain::StoreRevision, domain::RequestId)>;

    explicit EqStore(CommitHandler commitHandler = {});

    const domain::EqState& State() const noexcept;
    domain::StoreRevision Revision() const noexcept;

    UpdateResult SetParameter(const domain::SetEqParameterCommand& command);
    UpdateResult SetBypass(const domain::SetEqBypassCommand& command);
    UpdateResult ResetFilter(const domain::ResetEqFilterCommand& command);
    UpdateResult SetSectionBypass(const domain::SetEqSectionBypassCommand& command);
    UpdateResult ResetSection(const domain::ResetEqSectionCommand& command);
    UpdateResult AddBank(const domain::AddEqBankCommand& command);
    UpdateResult RemoveBank(const domain::RemoveEqBankCommand& command);
    UpdateResult RenameBank(const domain::RenameEqBankCommand& command);
    UpdateResult SelectBank(const domain::SelectEqBankCommand& command);
    UpdateResult ApplyFitResult(const domain::CompleteFitCommand& command);

    UpdateResult Replace(domain::EqState state, domain::StoreRevision revision);

private:
    models::FilterState* FindFilter(domain::BankId bankId, domain::FilterId filterId);
    const models::FilterDefinition* FindDefinition(domain::FilterId filterId) const;
    UpdateResult Commit(domain::RequestId requestId);
    UpdateResult Reject(const char* error) const;

    domain::EqState state;
    domain::StoreRevision revision = 0;
    long nextBankId = 2;
    domain::FilterDefinitionCatalog definitions;
    CommitHandler commitHandler;
};

} // namespace consolidator::host
