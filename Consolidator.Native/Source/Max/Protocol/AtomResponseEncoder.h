#pragma once

#include "MaxProtocolAdapter.h"

namespace consolidator::max
{

// Encodes typed Core responses into versioned Max output frames.
class AtomResponseEncoder
{
public:
    void Encode(
        const core::CommandResponse& response,
        c74::min::symbol source,
        std::uint64_t wireRequestId,
        MaxProtocolAdapter::FrameSink sink) const;
};

} // namespace consolidator::max
