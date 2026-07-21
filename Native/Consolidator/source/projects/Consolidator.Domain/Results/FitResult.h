#pragma once

#include "Ids/DomainIds.h"
#include "States/States.h"

#include <vector>

namespace consolidator::domain {

struct FitCandidate {
    BankId bankId{};
    std::vector<FilterState> filters;
    double loss = 0.0;
};

struct FitResult {
    SessionId sessionId{};
    BankId bankId{};
    std::vector<FilterState> filters;
    ProcessorState processor;
    double loss = 0.0;
};

} // namespace consolidator::domain
