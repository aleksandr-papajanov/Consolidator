#pragma once

#include "Commands/Commands.h"
#include "States/States.h"
#include "UpdateResult.h"

#include <functional>

namespace consolidator::host {

class SaturatorStore final {
public:
    using CommitHandler = std::function<void(domain::StoreRevision, domain::RequestId)>;

    explicit SaturatorStore(CommitHandler commitHandler = {});
    const domain::SaturatorState& State() const noexcept;
    domain::StoreRevision Revision() const noexcept;
    UpdateResult SetParameter(const domain::SetSaturatorParameterCommand& command);
    UpdateResult SetBypass(const domain::SetSaturatorBypassCommand& command);
    UpdateResult Reset(const domain::ResetSaturatorCommand& command);
    UpdateResult Replace(domain::SaturatorState state, domain::StoreRevision revision);
    bool CanApplyFit(const domain::SaturatorState& state) const noexcept;
    UpdateResult ApplyFit(domain::SaturatorState state, domain::RequestId requestId);

private:
    UpdateResult Commit(domain::RequestId requestId);
    UpdateResult Reject(const char* error) const;

    domain::SaturatorState state;
    domain::StoreRevision revision = 0;
    CommitHandler commitHandler;
};

} // namespace consolidator::host
