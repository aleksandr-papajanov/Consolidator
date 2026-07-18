#pragma once

#include <string_view>

namespace consolidator::models {

enum class FilterType {
    Gain,
    Tilt,
    Peak,
    LowShelf,
    HighShelf
};

inline std::string_view FilterTypeName(FilterType type) {
    switch (type) {
        case FilterType::Gain: return "gain";
        case FilterType::Tilt: return "tilt";
        case FilterType::Peak: return "peak";
        case FilterType::LowShelf: return "lowshelf";
        case FilterType::HighShelf: return "highshelf";
    }
    return "peak";
}

} // namespace consolidator::models
