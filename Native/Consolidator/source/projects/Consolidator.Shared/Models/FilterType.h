#pragma once

#include <string_view>

namespace consolidator::models {

enum class FilterType {
    Tilt,
    Peak,
    LowShelf,
    HighShelf
};

inline std::string_view FilterTypeName(FilterType type) {
    switch (type) {
        case FilterType::Tilt: return "tilt";
        case FilterType::Peak: return "peak";
        case FilterType::LowShelf: return "lowshelf";
        case FilterType::HighShelf: return "highshelf";
    }
    return {};
}

} // namespace consolidator::models
