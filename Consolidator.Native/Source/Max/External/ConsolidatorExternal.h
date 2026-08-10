#pragma once

#include <cstddef>
#include <memory>

namespace consolidator::core
{
class ConsolidatorInstance;
}

namespace consolidator::max
{

// Max-facing audio wrapper that forwards processing to one core instance.
class ConsolidatorExternal
{
public:
    ConsolidatorExternal();
    ~ConsolidatorExternal();

    ConsolidatorExternal(const ConsolidatorExternal&) = delete;
    ConsolidatorExternal& operator=(const ConsolidatorExternal&) = delete;
    ConsolidatorExternal(ConsolidatorExternal&&) = delete;
    ConsolidatorExternal& operator=(ConsolidatorExternal&&) = delete;

    // Processes the two-channel main/reference block exposed by the Max external.
    void Process(const double* mainInput,
                 const double* referenceInput,
                 double* mainOutput,
                 double* referenceOutput,
                 std::size_t frameCount);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    static constexpr std::size_t kChannelCount = 2;
};

} // namespace consolidator::max
