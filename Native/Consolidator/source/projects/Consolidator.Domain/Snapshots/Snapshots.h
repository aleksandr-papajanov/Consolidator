#pragma once

#include "Definitions/Definitions.h"
#include "Ids/DomainIds.h"
#include "States/States.h"

#include <vector>

namespace consolidator::domain {

struct DspSnapshot {
    StoreRevision revision = 0;
    EqState eq;
};

struct AnalyzerSnapshot {
    StoreRevision revision = 0;
    EqState eq;
    std::vector<double> selectedBankCurve;
    std::vector<double> selectedPrefixCurve;
    std::vector<double> totalCurve;
};

struct FitInputSnapshot {
    StoreRevision revision = 0;
    BankId bankId{};
    EqBank bank;
    FilterDefinitionCatalog definitions;
    std::vector<double> currentEqCurve;
};

} // namespace consolidator::domain
