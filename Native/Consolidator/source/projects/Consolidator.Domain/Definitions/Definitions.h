#pragma once

#include "Models/FilterDefinition.h"
#include "Settings/FilterOptions.h"

#include <map>

namespace consolidator::domain {

using FilterDefinition = models::FilterDefinition;
using FilterParameterDefinition = models::FilterParameterDefinition;
using FilterType = models::FilterType;
using ParameterRange = models::ParameterRange;
using ParameterScale = models::ParameterScale;
using FilterDefinitionCatalog = std::map<long, FilterDefinition>;

inline const FilterDefinitionCatalog& FilterDefinitions() {
    return settings::FilterOptions::EqDefinitions();
}

} // namespace consolidator::domain
