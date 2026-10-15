#pragma once

#include <string>

#include "Shared/Clients/ParameterEntry.h"

namespace consolidator::vst3::shared::presentation
{

class StateValueViewModel final
{
public:
    explicit StateValueViewModel(shared::clients::ParameterEntry entry);
    [[nodiscard]] bool Apply(shared::clients::ParameterEntry entry);
    [[nodiscard]] const shared::clients::ParameterEntry& Entry() const;
    [[nodiscard]] const std::string& Path() const;
    [[nodiscard]] const std::string& Label() const;
    void SetEnabled(bool enabled);
    [[nodiscard]] bool Enabled() const;

private:
    shared::clients::ParameterEntry entry_;
    std::string label_;
    bool enabled_{};
};

} // namespace consolidator::vst3::shared::presentation
