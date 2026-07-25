#pragma once

#include "Ids/DomainIds.h"

#include <string>
#include <optional>

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

enum class FitMode {
    Eq,
    Dynamics,
    Saturation
};

inline const char* FitModeName(FitMode mode) noexcept {
    switch (mode) {
        case FitMode::Eq: return "eq";
        case FitMode::Dynamics: return "dynamics";
        case FitMode::Saturation: return "saturation";
    }
    return "eq";
}

inline std::optional<FitMode> ParseFitMode(const std::string& value) {
    if (value == "eq") return FitMode::Eq;
    if (value == "dynamics") return FitMode::Dynamics;
    if (value == "saturation") return FitMode::Saturation;
    return std::nullopt;
}

inline std::string FitOperationName(FitMode mode) {
    return std::string{ "fit." } + FitModeName(mode);
}

struct OperationError {
    std::string code;
    std::string message;
};

} // namespace consolidator::domain
