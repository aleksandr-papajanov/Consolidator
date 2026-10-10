#include "TestSupport.h"

int main()
{
    const auto abiSucceeded =
        consolidator::tests::RunAbiContractTests();
    const auto persistenceBlobSucceeded =
        consolidator::tests::RunPersistenceBlobCodecTests();
    const auto persistenceRestoreGateSucceeded =
        consolidator::tests::RunPersistenceRestoreGateTests();

    if (!abiSucceeded ||
        !persistenceBlobSucceeded ||
        !persistenceRestoreGateSucceeded)
    {
        return 1;
    }

    std::cout << "Consolidator.Native.Tests passed\n";
    return 0;
}
