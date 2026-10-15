#pragma once

#include <string>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::features::equalizer::bindings
{

struct ParameterEdit
{
    std::string path;
    shared::protocol::ProtocolValue value;
};

} // namespace consolidator::vst3::features::equalizer::bindings
