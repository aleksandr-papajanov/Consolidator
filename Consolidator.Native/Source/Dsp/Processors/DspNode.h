#pragma once

#include "Dsp/Parameters/ParameterRoute.h"
#include "Dsp/Parameters/ParameterValue.h"

namespace consolidator::dsp
{

class DspNode
{
public:
    virtual ~DspNode() = default;

    virtual bool ApplyParameter(
        const ParameterRoute& route,
        const ParameterValue& value,
        std::size_t depth) = 0;
};

} // namespace consolidator::dsp
