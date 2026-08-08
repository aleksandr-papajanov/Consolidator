#include "ConsolidatorExternal.h"

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::max
{

struct ConsolidatorExternal::Impl
{
    consolidator::core::ConsolidatorInstance instance;
};

ConsolidatorExternal::ConsolidatorExternal()
    : impl_(std::make_unique<Impl>())
{
}

ConsolidatorExternal::~ConsolidatorExternal() = default;

void ConsolidatorExternal::Process(const double* mainInput,
                                   const double* referenceInput,
                                   double* mainOutput,
                                   double* referenceOutput,
                                   std::size_t frameCount)
{
    impl_->instance.Process(mainInput, referenceInput, mainOutput, referenceOutput, frameCount);
}

} // namespace consolidator::max
