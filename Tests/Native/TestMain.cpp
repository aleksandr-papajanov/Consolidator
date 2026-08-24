#include "TestSupport.h"

int main()
{
    const auto abiSucceeded =
        consolidator::tests::RunAbiContractTests();
    const auto dspStateConsumerSucceeded =
        consolidator::tests::RunDspStateConsumerTests();

    if (!abiSucceeded || !dspStateConsumerSucceeded)
    {
        return 1;
    }

    std::cout << "Consolidator.Native.Tests passed\n";
    return 0;
}
