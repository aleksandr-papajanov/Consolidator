#pragma once

#include <array>
#include <cstddef>

struct FrequencyBand final {
    double minimumHz;
    double maximumHz;
};

class FrequencyBands final {
public:
    static constexpr std::size_t Count = 6;
    static constexpr std::array<FrequencyBand, Count> Standard{{
        { 20.0, 80.0 }, { 80.0, 250.0 }, { 250.0, 700.0 },
        { 700.0, 2000.0 }, { 2000.0, 6000.0 }, { 6000.0, 20000.0 }
    }};
};
