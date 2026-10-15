#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "Shared/Clients/ParameterEntry.h"

namespace consolidator::vst3::features::equalizer::clients
{

using BankSnapshots = std::array<std::vector<shared::clients::ParameterEntry>, 7>;

struct AnalyzerSpectrum
{
    std::size_t fftSize;
    std::vector<double> source;
    std::vector<double> group;
    std::vector<double> reference;
};

} // namespace consolidator::vst3::features::equalizer::clients
