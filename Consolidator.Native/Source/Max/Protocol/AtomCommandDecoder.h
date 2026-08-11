#pragma once

#include "MaxProtocolAdapter.h"

namespace consolidator::max
{

// Decodes versioned Max frames into typed Core commands.
class AtomCommandDecoder
{
public:
    DecodeResult Decode(
        c74::min::symbol selector,
        const c74::min::atoms& args,
        core::InstanceId instance,
        core::RequestId requestId) const;
};

} // namespace consolidator::max
