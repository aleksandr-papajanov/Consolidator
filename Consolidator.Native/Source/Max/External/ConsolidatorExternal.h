#pragma once

#include <cstddef>
#include <memory>

namespace consolidator::core
{
class ConsolidatorInstance;
}

namespace consolidator::max
{

class ConsolidatorExternal
{
public:
    ConsolidatorExternal();
    ~ConsolidatorExternal();

    ConsolidatorExternal(const ConsolidatorExternal&) = delete;
    ConsolidatorExternal& operator=(const ConsolidatorExternal&) = delete;
    ConsolidatorExternal(ConsolidatorExternal&&) = delete;
    ConsolidatorExternal& operator=(ConsolidatorExternal&&) = delete;

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