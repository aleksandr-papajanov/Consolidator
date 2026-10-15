#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"
#include "Shared/Controls/Dial/DialViewModel.h"
#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Presentation/StateValueViewModel.h"

namespace consolidator::vst3::shared::controls::parameter::viewmodels
{

class ParameterViewModel final
{
public:
    ParameterViewModel(shared::clients::ParameterEntry entry,
        const shared::controls::dial::DialRuntimeOverlayViewModel* runtimeOverlay,
        std::span<const std::string_view> choices);
    [[nodiscard]] bool Apply(shared::clients::ParameterEntry entry);
    [[nodiscard]] shared::presentation::StateValueViewModel& State();
    [[nodiscard]] const shared::presentation::StateValueViewModel& State() const;
    [[nodiscard]] shared::controls::dial::DialViewModel* Dial();
    [[nodiscard]] bool HasChoices() const;
    [[nodiscard]] int ChoiceValue() const;
    [[nodiscard]] std::span<const std::string_view> Choices() const;
    [[nodiscard]] bool ButtonValue() const;
    void SetScope(bool groupScope);
    [[nodiscard]] bool GroupScope() const;

private:
    void ValidateDiscrete() const;

    shared::presentation::StateValueViewModel state_;
    std::unique_ptr<shared::controls::dial::DialViewModel> dial_;
    std::span<const std::string_view> choices_;
    bool groupScope_{};
};

} // namespace consolidator::vst3::shared::controls::parameter::viewmodels
