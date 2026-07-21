#pragma once

#include <string_view>

namespace consolidator::models {

enum class EqSection {
    Pre,
    Post
};

inline std::string_view EqSectionName(EqSection section) {
    return section == EqSection::Pre ? "pre" : "post";
}

} // namespace consolidator::models
