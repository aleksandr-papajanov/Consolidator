#include "TestSupport.h"

int main()
{
    const auto abiSucceeded =
        consolidator::tests::RunAbiContractTests();
    const auto persistenceBlobSucceeded =
        consolidator::tests::RunPersistenceBlobCodecTests();
    const auto persistenceRestoreGateSucceeded =
        consolidator::tests::RunPersistenceRestoreGateTests();
    const auto dspParameterSmootherSucceeded =
        consolidator::tests::RunDspParameterSmootherTests();
    const auto dspStateConsumerSucceeded =
        consolidator::tests::RunDspStateConsumerTests();

    if (!abiSucceeded ||
        !persistenceBlobSucceeded ||
        !persistenceRestoreGateSucceeded ||
        !dspParameterSmootherSucceeded ||
        !dspStateConsumerSucceeded)
    {
        return 1;
    }

    std::cout << "Consolidator.Native.Tests passed\n";
    return 0;
}
