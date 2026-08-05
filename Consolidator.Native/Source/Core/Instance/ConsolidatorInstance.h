#pragma once

#include <cstddef>

#include "Dsp/Parameters/ParameterChange.h"

namespace consolidator::core
{

class ConsolidatorInstance
{
public:
    ConsolidatorInstance();
    ~ConsolidatorInstance();

    ConsolidatorInstance(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance& operator=(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance(ConsolidatorInstance&&) = delete;
    ConsolidatorInstance& operator=(ConsolidatorInstance&&) = delete;

    void Process(const double* mainInput,
                 const double* referenceInput,
                 double* mainOutput,
                 double* referenceOutput,
                 std::size_t frameCount);

    void ApplyParameterChange(const dsp::ParameterChange& change);

private:
    static constexpr std::size_t kChannelCount = 2;
};

} // namespace consolidator::core
