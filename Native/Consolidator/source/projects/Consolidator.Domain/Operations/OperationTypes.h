#pragma once

#include "Ids/DomainIds.h"

#include <string>

namespace consolidator::domain {

enum class OperationStatus {
    Idle = 0,
    Starting = 1,
    Capturing = 2,
    Processing = 3,
    Validating = 4,
    Applying = 5,
    Completed = 6,
    Cancelled = 7,
    Failed = 8
};

struct OperationError {
    std::string code;
    std::string message;
};

} // namespace consolidator::domain
