#pragma once

#include <array>
#include <vector>
#include <cmath>

struct BellParams {
    double gainDb = 0.0;
    double freqHz = 1000.0;
    double q = 1.0;
};

struct ShelfParams {
    double gainDb = 0.0;
    double freqHz = 200.0;
    double q = 0.707;
};

struct EqParams {
    double gainDb = 0.0;

    double tiltDb = 0.0;
    double tiltPivotHz = 1000.0;

    ShelfParams lowShelf;
    ShelfParams highShelf;

    std::array<BellParams, 4> bells;
};