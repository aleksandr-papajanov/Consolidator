#pragma once

namespace consolidator::dsp
{

// Authoritative user control that is not a DSP parameter.
template <typename T>
struct StateMarker
{
    T value{};
};

} // namespace consolidator::dsp
