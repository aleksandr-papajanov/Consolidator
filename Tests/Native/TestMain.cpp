#include "TestSupport.h"

int main()
{
    const auto abiSucceeded =
        consolidator::tests::RunAbiContractTests();
    const auto persistenceBlobSucceeded =
        consolidator::tests::RunPersistenceBlobCodecTests();
    const auto persistenceRestoreGateSucceeded =
        consolidator::tests::RunPersistenceRestoreGateTests();
    const auto dspParameterRampSucceeded =
        consolidator::tests::RunDspParameterRampTests();
    const auto dspRuntimeSucceeded =
        consolidator::tests::RunDspRuntimeTests();
    const auto dspStateConsumerSucceeded =
        consolidator::tests::RunDspStateConsumerTests();
    const auto dspRuntimeGraphSucceeded =
        consolidator::tests::RunDspRuntimeGraphTests();
    const auto levelerProcessorSucceeded =
        consolidator::tests::RunLevelerProcessorTests();
    const auto polishAirProcessorSucceeded =
        consolidator::tests::RunPolishAirProcessorTests();
    const auto polishThickProcessorSucceeded =
        consolidator::tests::RunPolishThickProcessorTests();

    if (!abiSucceeded ||
        !persistenceBlobSucceeded ||
        !persistenceRestoreGateSucceeded ||
        !dspParameterRampSucceeded ||
        !dspRuntimeSucceeded ||
        !dspStateConsumerSucceeded ||
        !dspRuntimeGraphSucceeded ||
        !levelerProcessorSucceeded ||
        !polishAirProcessorSucceeded ||
        !polishThickProcessorSucceeded)
    {
        return 1;
    }

    std::cout << "Consolidator.Native.Tests passed\n";
    return 0;
}
