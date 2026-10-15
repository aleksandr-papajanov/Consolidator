#include "Shared/Controls/Parameter/Bindings/ParameterBindings.h"

#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace consolidator::vst3::shared::controls::parameter::bindings
{

ParameterBindings::ParameterBindings(const shared::clients::TargetStateClient& state,
    shared::editing::DialGestureSession& gesture, ParameterBindingActions actions)
    : state_(state), gesture_(gesture), actions_(std::move(actions))
{
}

const std::vector<std::unique_ptr<shared::controls::parameter::viewmodels::ParameterViewModel>>& ParameterBindings::Models() const
{
    return models_;
}

std::vector<shared::protocol::StateWriteEntry> ParameterBindings::Write(const std::string& path,
    shared::protocol::ProtocolValue value) const
{
    return { { path, std::move(value), state_.WriteMode(path, groupScope_) } };
}

shared::editing::ParameterIntents ParameterBindings::Intents(const std::string& path)
{
    shared::editing::ParameterIntents intents;
    intents.commit = actions_.commit;
    intents.reset = actions_.reset;
    intents.begin = [this, path](const std::string&, shared::protocol::ProtocolValue value)
    {
        return actions_.begin(path, Write(path, std::move(value)));
    };
    intents.change = [this, path](shared::protocol::ProtocolValue value)
    {
        auto entries = Write(path, std::move(value));
        actions_.preview(entries);
        gesture_.SetValues(std::move(entries));
    };
    intents.end = [this, path](shared::protocol::ProtocolValue value)
    {
        auto entries = Write(path, std::move(value));
        actions_.preview(entries);
        gesture_.End(std::move(entries));
        actions_.ended();
    };
    return intents;
}

void ParameterBindings::ApplySnapshot(
    std::span<const shared::controls::parameter::presentation::ParameterPresentation> presentations,
    shared::controls::dial::DialRuntimeOverlayBinding runtimeOverlay)
{
    if (runtimeOverlay.path.empty() != (runtimeOverlay.model == nullptr))
    {
        throw std::invalid_argument("Incomplete dial runtime overlay binding.");
    }
    std::vector<std::unique_ptr<shared::controls::parameter::viewmodels::ParameterViewModel>> models;
    for (const auto& presentation : presentations)
    {
        for (const auto path : presentation.orderedPaths)
        {
            const auto& entry = state_.Require(std::string(path));
            const auto* overlay = std::string_view(entry.path) == runtimeOverlay.path
                ? runtimeOverlay.model : nullptr;
            auto model = std::make_unique<shared::controls::parameter::viewmodels::ParameterViewModel>(entry,
                overlay, presentation.ChoicesFor(entry.path));
            model->SetScope(groupScope_);
            models.push_back(std::move(model));
        }
    }

    models_ = std::move(models);
}

bool ParameterBindings::ApplyChanged(const shared::clients::ParameterEntry& entry)
{
    actions_.valueAccepted(entry);
    for (const auto& model : models_)
    {
        if (model->State().Path() == entry.path)
        {
            return model->Apply(entry);
        }
    }
    return false;
}

void ParameterBindings::SetScope(bool groupScope)
{
    groupScope_ = groupScope;
    for (const auto& model : models_)
    {
        model->SetScope(groupScope);
    }
}

void ParameterBindings::SetEditing(bool enabled)
{
    for (const auto& model : models_)
    {
        const bool editable = enabled && (!gesture_.Active() ||
            (gesture_.Editing() && gesture_.Path() == model->State().Path()));
        model->State().SetEnabled(editable);
    }
}

void ParameterBindings::FinishPreview()
{
    for (const auto& model : models_)
    {
        if (auto* dial = model->Dial())
        {
            dial->FinishPreview();
        }
    }
}

} // namespace consolidator::vst3::shared::controls::parameter::bindings
