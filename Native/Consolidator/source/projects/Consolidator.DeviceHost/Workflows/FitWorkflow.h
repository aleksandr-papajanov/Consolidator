#pragma once

#include "Commands/Commands.h"
#include "EqStore.h"
#include "Events/Events.h"
#include "States/States.h"

#include <functional>
#include <string>

namespace consolidator::host {

class FitWorkflow final {
public:
    using EventHandler = std::function<void(domain::Event)>;

    FitWorkflow(EqStore& eqStore, EventHandler eventHandler = {});

    void Handle(const domain::StartFitCommand& command);
    void Handle(const domain::CancelFitCommand& command);
    void Handle(const domain::ClearFitCommand& command);
    void Handle(const domain::CompleteFitCommand& command);
    void Handle(const domain::FailFitCommand& command);
    const domain::ApproximatorState& State() const noexcept;

private:
    void Reject(domain::RequestId requestId, const std::string& error);
    void Fail(domain::RequestId requestId, const std::string& error, bool rejectCommand);
    void Publish(domain::Event event) const;

    EqStore& eqStore;
    domain::ApproximatorState state;
    domain::BankId activeBankId{};
    EventHandler eventHandler;
};

} // namespace consolidator::host
