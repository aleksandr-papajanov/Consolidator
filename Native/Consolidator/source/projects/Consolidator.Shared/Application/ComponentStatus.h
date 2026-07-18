#pragma once

#include <string_view>

namespace consolidator::application {

enum class ComponentStatus {
    Initializing,
    Ready,
    Processing
};

constexpr std::string_view ComponentStatusName(ComponentStatus status) {
    switch (status) {
        case ComponentStatus::Initializing: return "initializing";
        case ComponentStatus::Ready: return "ready";
        case ComponentStatus::Processing: return "processing";
    }
    return "initializing";
}

} // namespace consolidator::application
