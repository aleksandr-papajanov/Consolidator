#pragma once

#include "Commands/Commands.h"
#include "States/States.h"
#include "UpdateResult.h"

#include <functional>

namespace consolidator::host {

class GainStore final {
public:
    using CommitHandler = std::function<void(domain::StoreRevision, domain::RequestId)>;

    explicit GainStore(CommitHandler commitHandler = {});

    const domain::GainState& State() const noexcept;
    domain::StoreRevision Revision() const noexcept;
    UpdateResult SetParameter(const domain::SetGainParameterCommand& command);
    UpdateResult Replace(domain::GainState state, domain::StoreRevision revision);

private:
    UpdateResult Commit(domain::RequestId requestId);
    UpdateResult Reject(const char* error) const;

    domain::GainState state;
    domain::StoreRevision revision = 0;
    CommitHandler commitHandler;
};

} // namespace consolidator::host
