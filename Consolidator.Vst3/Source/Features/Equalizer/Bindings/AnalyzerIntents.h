#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Features/Equalizer/Bindings/ParameterEdit.h"

namespace consolidator::vst3::features::equalizer::bindings
{

struct AnalyzerIntents
{
    std::function<bool(const std::string&, std::vector<ParameterEdit>)> begin;
    std::function<void(std::vector<ParameterEdit>)> change;
    std::function<void(std::vector<ParameterEdit>)> end;
    std::function<void(const std::string&)> reset;
    std::function<void(const std::string&, bool)> bypass;
};

} // namespace consolidator::vst3::features::equalizer::bindings
