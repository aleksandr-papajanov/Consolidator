#include "TestSupport.h"

#include "Transport/PersistenceRestoreGate.h"

namespace consolidator::tests
{

bool RunPersistenceRestoreGateTests()
{
    consolidator::max::transport::PersistenceRestoreGate gate;

    if (!Expect(
            gate.ShouldRestore(),
            "Persistence restore was blocked outside a local notification."))
    {
        return false;
    }

    gate.BeginLocalChangeNotification();
    if (!Expect(
            !gate.ShouldRestore(),
            "Persistence self-restore was allowed during a local notification."))
    {
        return false;
    }

    gate.EndLocalChangeNotification();
    return Expect(
        gate.ShouldRestore(),
        "Persistence restore remained blocked after a local notification.");
}

} // namespace consolidator::tests
