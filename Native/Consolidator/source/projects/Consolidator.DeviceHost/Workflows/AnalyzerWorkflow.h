#pragma once

#include "Commands/Commands.h"
#include "Events/Events.h"
#include "States/States.h"

#include <functional>

namespace consolidator::host {

class AnalyzerWorkflow final {
public:
    using EventHandler = std::function<void(domain::Event)>;

    explicit AnalyzerWorkflow(EventHandler eventHandler = {});

    void Handle(const domain::ListenAnalyzerCommand& command);
    const domain::AnalyzerState& State() const noexcept;

private:
    void Publish(domain::Event event) const;

    domain::AnalyzerState state;
    EventHandler eventHandler;
};

} // namespace consolidator::host
