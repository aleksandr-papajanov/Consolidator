#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Editing/ParameterIntents.h"
#include "Shared/Clients/TargetStateClient.h"
#include "Shared/Editing/DialGestureSession.h"
#include "Shared/Protocol/ProtocolFrame.h"
#include "Shared/Controls/Parameter/Presentation/ParameterPresentation.h"
#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"
#include "Shared/Controls/Parameter/ViewModels/ParameterViewModel.h"

namespace consolidator::vst3::shared::controls::parameter::bindings
{

struct ParameterBindingActions
{
    std::function<void(const std::string&, std::int64_t)> commit;
    std::function<bool(const std::string&, std::vector<shared::protocol::StateWriteEntry>)> begin;
    std::function<void()> ended;
    std::function<void(const std::string&)> reset;
    std::function<void(const std::vector<shared::protocol::StateWriteEntry>&)> preview;
    std::function<void(const shared::clients::ParameterEntry&)> valueAccepted;
};

class ParameterBindings final
{
public:
    ParameterBindings(const shared::clients::TargetStateClient& state, shared::editing::DialGestureSession& gesture,
        ParameterBindingActions actions);
    [[nodiscard]] const std::vector<std::unique_ptr<shared::controls::parameter::viewmodels::ParameterViewModel>>& Models() const;
    [[nodiscard]] shared::editing::ParameterIntents Intents(const std::string& path);
    void ApplySnapshot(std::span<const shared::controls::parameter::presentation::ParameterPresentation> presentations,
        std::span<const shared::controls::dial::DialRuntimeOverlayBinding> runtimeOverlays);
    [[nodiscard]] bool ApplyChanged(const shared::clients::ParameterEntry& entry);
    void SetScope(bool groupScope);
    void SetEditing(bool enabled);
    void FinishPreview();

private:
    [[nodiscard]] std::vector<shared::protocol::StateWriteEntry> Write(const std::string& path,
        shared::protocol::ProtocolValue value) const;

    const shared::clients::TargetStateClient& state_;
    shared::editing::DialGestureSession& gesture_;
    ParameterBindingActions actions_;
    std::vector<std::unique_ptr<shared::controls::parameter::viewmodels::ParameterViewModel>> models_;
    bool groupScope_{};
};

} // namespace consolidator::vst3::shared::controls::parameter::bindings
