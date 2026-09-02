#include "TestSupport.h"

#include "DspStateConsumer.h"

namespace consolidator::tests
{

bool RunDspStateConsumerTests()
{
    consolidator::max::DspStateExchange exchange{};
    exchange.snapshots[1].inputLevel = 2.0F;
    exchange.publishedIndex = 1;
    std::uint32_t consumerIndex = 0;
    consolidator::max::DspSnapshot local{};

    auto succeeded = true;
    succeeded &= Expect(
        consolidator::max::ConsumePublishedDspState(
            exchange,
            consumerIndex,
            local),
        "A newly published DSP snapshot was not consumed.");
    succeeded &= Expect(
        consumerIndex == 1 &&
            exchange.consumerIndex == 1 &&
            local.inputLevel == 2.0F,
        "DSP snapshot claim and copy were not completed as one consume operation.");

    exchange.snapshots[1].inputLevel = 5.0F;
    succeeded &= Expect(
        !consolidator::max::ConsumePublishedDspState(
            exchange,
            consumerIndex,
            local) &&
            local.inputLevel == 2.0F,
        "An unchanged publication index was consumed twice.");

    exchange.publishedIndex = 7;
    succeeded &= Expect(
        !consolidator::max::ConsumePublishedDspState(
            exchange,
            consumerIndex,
            local) &&
            consumerIndex == 1 &&
            local.inputLevel == 2.0F,
        "An invalid publication index changed native DSP state.");
    return succeeded;
}

} // namespace consolidator::tests
