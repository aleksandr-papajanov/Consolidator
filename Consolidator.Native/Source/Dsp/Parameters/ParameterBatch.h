#pragma once

#include <span>
#include <vector>

#include "Dsp/Parameters/ParameterChange.h"

namespace consolidator::dsp
{

class ParameterBatch
{
public:
    void Add(ParameterChange change)
    {
        changes_.push_back(change);
    }

    void Clear() noexcept
    {
        changes_.clear();
    }

    [[nodiscard]] bool IsEmpty() const noexcept
    {
        return changes_.empty();
    }

    [[nodiscard]] std::size_t GetSize() const noexcept
    {
        return changes_.size();
    }

    [[nodiscard]] std::span<const ParameterChange> GetChanges() const noexcept
    {
        return changes_;
    }

private:
    std::vector<ParameterChange> changes_;
};

} // namespace consolidator::dsp
