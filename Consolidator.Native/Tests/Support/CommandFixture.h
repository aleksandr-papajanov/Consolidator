#pragma once

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Registry/RegistryState.h"
#include "Core/Routing/CommandRouter.h"
#include "Core/Routing/GroupGraph.h"
#include "Core/Routing/ParameterConstraintResolver.h"
#include "Core/Routing/StateRouter.h"
#include "Core/Routing/StateWriter.h"

namespace consolidator::test
{

// Composes command routing synchronously while retaining a fully initialized
// instance and its registered audio-thread mailboxes.
class CommandFixture
{
public:
    CommandFixture()
        : groups(registry)
        , stateRouter(registry, groups)
        , constraints(registry, stateRouter)
        , stateWriter(registry, stateRouter, constraints)
        , commandRouter(registry, registryState, constraints, stateWriter)
    {
        instance.Initialize();
        registry.RegisterInstance(instance.GetInstanceId(), &instance);
        (void)registryState.Refresh(registry);
    }

    core::InstanceRegistry registry;
    core::RegistryState registryState;
    core::GroupGraph groups;
    core::StateRouter stateRouter;
    core::ParameterConstraintResolver constraints;
    core::StateWriter stateWriter;
    core::CommandRouter commandRouter;
    core::ConsolidatorInstance instance;
};

} // namespace consolidator::test
