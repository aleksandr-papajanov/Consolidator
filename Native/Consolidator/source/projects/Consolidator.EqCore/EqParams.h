#pragma once

#include <array>

struct BellParams {
    double gainDb = 0.0;
    double freqHz = 1000.0;
    double q = 1.0;

    friend constexpr bool operator==(const BellParams&, const BellParams&) = default;
};

struct ShelfParams {
    double gainDb = 0.0;
    double freqHz = 200.0;
    double q = 0.707;

    friend constexpr bool operator==(const ShelfParams&, const ShelfParams&) = default;
};

struct EqParams {
    double gainDb = 0.0;

    double tiltDb = 0.0;
    double tiltPivotHz = 1000.0;

    ShelfParams lowShelf;
    ShelfParams highShelf;

    std::array<BellParams, 4> bells;

    friend constexpr bool operator==(const EqParams&, const EqParams&) = default;
};
