#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Shared/Clients/RegistryClient.h"
#include "Shared/Clients/TrackLevelClient.h"
#include "Features/Tracks/ViewModels/TrackFeedback.h"

namespace consolidator::vst3::features::tracks::viewmodels
{

struct BankCellPresentation
{
    bool selected;
    bool focused;
    bool highlighted;
};

struct TrackActions
{
    bool group;
    bool ungroup;
    bool clear;
    bool process;
    bool scope;
};

class TracksViewModel final
{
public:
    TracksViewModel(const shared::clients::RegistryClient& registry, const shared::clients::TrackLevelClient& levels,
        std::uint64_t localInstance);
    void Refresh();
    void ConfirmFeedback();
    [[nodiscard]] TrackFeedback& Feedback();
    void SetFocus(std::uint64_t instance, int bank, bool groupScope);
    void ClearSelection();
    void Select(std::uint64_t instance, int bank);
    [[nodiscard]] bool ToggleSelection(std::uint64_t instance, int bank);
    [[nodiscard]] const std::map<std::uint64_t, int>& SelectedBanks() const;
    [[nodiscard]] const std::vector<shared::clients::RegistryInstance>& Rows() const;
    [[nodiscard]] const shared::clients::RegistryBank* FindBank(std::uint64_t instance, int bank) const;
    [[nodiscard]] BankCellPresentation BankCell(std::uint64_t instance,
        const shared::clients::RegistryBank& bank) const;
    [[nodiscard]] bool Local(std::uint64_t instance) const;
    [[nodiscard]] bool InstanceFocused(std::uint64_t instance) const;
    [[nodiscard]] int FocusedBank() const;
    [[nodiscard]] std::uint64_t FocusedInstance() const;
    [[nodiscard]] std::optional<std::uint32_t> FocusedGroup() const;
    [[nodiscard]] bool GroupScope() const;
    [[nodiscard]] TrackActions Actions() const;
    [[nodiscard]] static double OutputAlpha(double outputLevelDb);
    [[nodiscard]] double OutputAlphaFor(std::uint64_t instance) const;
    [[nodiscard]] static std::string GroupLabel(std::uint32_t group);
    [[nodiscard]] static std::string Label(const shared::clients::RegistryInstance& instance);

private:
    const shared::clients::RegistryClient& registry_;
    const shared::clients::TrackLevelClient& levels_;
    std::uint64_t localInstance_;
    std::map<std::uint64_t, int> selected_;
    std::uint64_t focusedInstance_{};
    int focusedBank_{};
    bool groupScope_{};
    TrackFeedback feedback_;
};

} // namespace consolidator::vst3::features::tracks::viewmodels
