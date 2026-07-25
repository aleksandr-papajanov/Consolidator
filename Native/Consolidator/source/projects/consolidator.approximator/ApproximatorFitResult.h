#pragma once

#include "Snapshots/Snapshots.h"

struct ApproximatorFitResult final {
    consolidator::domain::DspSnapshot snapshot;
    double loss = 0.0;
};
