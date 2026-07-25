#pragma once

#include "Commands/Commands.h"
#include "States/States.h"
#include "UpdateResult.h"

#include <functional>

namespace consolidator::host {

class CompressorStore final {
public:
    using CommitHandler = std::function<void(domain::StoreRevision, domain::RequestId)>;

    explicit CompressorStore(CommitHandler commitHandler = {});

    const domain::CompressorState& State() const noexcept;
    domain::StoreRevision Revision() const noexcept;
    UpdateResult SetParameter(const domain::SetCompressorParameterCommand& command);
    UpdateResult SetBypass(const domain::SetCompressorBypassCommand& command);
    UpdateResult SetMode(const domain::SetCompressorModeCommand& command);
    UpdateResult SetDetectorParameter(const domain::SetCompressorDetectorParameterCommand& command);
    UpdateResult SetDetectorListen(const domain::SetCompressorDetectorListenCommand& command);
    UpdateResult Reset(const domain::ResetCompressorCommand& command);
    UpdateResult Replace(domain::CompressorState state, domain::StoreRevision revision);
    bool CanApplyFit(const domain::CompressorState& state) const noexcept;
    UpdateResult ApplyFit(domain::CompressorState state, domain::RequestId requestId);

private:
    UpdateResult Commit(domain::RequestId requestId);
    UpdateResult Reject(const char* error) const;

    domain::CompressorState state;
    domain::StoreRevision revision = 0;
    CommitHandler commitHandler;
};

} // namespace consolidator::host
