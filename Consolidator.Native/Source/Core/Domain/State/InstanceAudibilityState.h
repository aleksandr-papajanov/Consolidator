#pragma once

#include "Core/Domain/State/StateMarker.h"

namespace consolidator::dsp
{

struct InstanceAudibilityState
{
    StateMarker<bool> mute;
    StateMarker<bool> solo;
};

} // namespace consolidator::dsp
