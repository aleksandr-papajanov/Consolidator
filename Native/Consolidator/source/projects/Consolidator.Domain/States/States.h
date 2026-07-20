#pragma once

#include "Models/EqSnapshot.h"
#include "Models/FilterState.h"
#include "Ids/DomainIds.h"

#include <optional>
#include <string>

namespace consolidator::domain {

using FilterState = models::FilterState;
using EqBank = models::EqBank;
using EqState = models::EqSnapshot;

struct AnalyzerState {
    enum class Status { Idle, Listening, Processing, Completed, Failed };

    Status status = Status::Idle;
    SessionId sessionId{};
    std::uint64_t framesProcessed = 0;
    double progress = 0.0;
    std::string error;
};

struct ApproximatorState {
    enum class Status { Idle, Ready, Processing, Completed, Failed };

    Status status = Status::Idle;
    SessionId sessionId{};
    double progress = 0.0;
    double loss = 0.0;
    std::string error;
};

struct DeviceState {
    EqState eq;
    AnalyzerState analyzer;
    ApproximatorState approximator;
};

} // namespace consolidator::domain
