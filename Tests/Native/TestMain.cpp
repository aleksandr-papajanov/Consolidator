#include "TestSupport.h"

int main()
{
    const auto abiSucceeded =
        consolidator::tests::RunAbiContractTests();
    const auto persistenceBlobSucceeded =
        consolidator::tests::RunPersistenceBlobCodecTests();
    const auto persistenceRestoreGateSucceeded =
        consolidator::tests::RunPersistenceRestoreGateTests();
    const auto dspRuntimeSucceeded =
        consolidator::tests::RunDspRuntimeTests();
    const auto dspStateConsumerSucceeded =
        consolidator::tests::RunDspStateConsumerTests();
    const auto dspRuntimeGraphSucceeded =
        consolidator::tests::RunDspRuntimeGraphTests();

    if (!abiSucceeded ||
        !persistenceBlobSucceeded ||
        !persistenceRestoreGateSucceeded ||
        !dspRuntimeSucceeded ||
        !dspStateConsumerSucceeded ||
        !dspRuntimeGraphSucceeded)
    {
        return 1;
    }

    std::cout << "Consolidator.Native.Tests passed\n";
    return 0;
}
