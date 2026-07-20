#pragma once

#include <string>

namespace consolidator::host {

enum class UpdateStatus {
    Changed,
    Unchanged,
    Rejected
};

struct UpdateResult {
    UpdateStatus status = UpdateStatus::Unchanged;
    std::string error;

    bool Changed() const noexcept { return status == UpdateStatus::Changed; }
    bool Accepted() const noexcept { return status != UpdateStatus::Rejected; }
};

} // namespace consolidator::host
