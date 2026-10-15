#include "Features/Tracks/Controls/TrackListControl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <unordered_set>

#include "Features/Tracks/Controls/BankCellRenderer.h"

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::tracks::controls
{

using features::tracks::bindings::TrackIntentKind;

namespace
{
constexpr int kProcessorSlotWidth = 12;
constexpr int kProcessorMarkerSize = 8;
constexpr int kLevelMarkerSize = 9;
constexpr int kRowHeight = shared::ui::kBankCellSize;
constexpr int kHorizontalPadding = 4;
constexpr int kGap = 5;
const std::array<std::string, 6> kProcessors{ "input", "saturator", "compressor", "equalizer", "polish", "output" };

}

TrackListControl::TrackListControl(
    features::tracks::viewmodels::TracksViewModel& model,
    std::function<void(const features::tracks::bindings::TrackIntent&)> emit)
    : model_(model), emit_(std::move(emit)), bankInteraction_(model, emit_)
{
}

void TrackListControl::Refresh()
{
    SyncRowButtons();
    RefreshPresentation();
    ScrollBy(0.0F);
    repaint();
}

void TrackListControl::SyncRowButtons()
{
    std::unordered_set<std::uint64_t> instances;
    instances.reserve(model_.Rows().size());
    for (const auto& row : model_.Rows())
    {
        instances.insert(row.id);
        auto [found, created] = rowButtons_.try_emplace(row.id);
        if (created)
        {
            found->second = std::make_unique<InstanceButtonsControl>(model_, emit_, row.id);
            addAndMakeVisible(*found->second);
        }
    }
    std::erase_if(rowButtons_, [&instances](const auto& row) { return !instances.contains(row.first); });
}

void TrackListControl::RefreshPresentation()
{
    for (const auto& row : rowButtons_)
    {
        row.second->RefreshPresentation();
    }
    repaint();
}

void TrackListControl::PositionRowButtons()
{
    const auto layout = GetLayout();
    const auto& rows = model_.Rows();
    for (std::size_t row = 0; row < rows.size(); ++row)
    {
        const auto found = rowButtons_.find(rows[row].id);
        if (found == rowButtons_.end())
        {
            continue;
        }
        const int top = static_cast<int>(row) * kRowHeight - static_cast<int>(scroll_);
        auto& buttons = *found->second;
        buttons.SetVerticalConnections(row > 0, row + 1 < rows.size());
        buttons.setBounds(layout.controls, top, 4 * shared::ui::kBankCellSize, shared::ui::kBankCellSize);
        buttons.setVisible(top + kRowHeight > 0 && top < getHeight());
    }
}

TrackListControl::Layout TrackListControl::GetLayout() const
{
    const auto controls = getWidth() - kHorizontalPadding - 4 * shared::ui::kBankCellSize;
    const auto banks = controls - kGap - 7 * shared::ui::kBankCellSize;
    const auto markers = banks - kGap - static_cast<int>(kProcessors.size()) * kProcessorSlotWidth;
    return { markers, banks, controls, std::max(0, markers - kGap - kHorizontalPadding) };
}

void TrackListControl::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kBackground);
    graphics.setFont(shared::ui::ControlFont());
    const auto layout = GetLayout();
    const auto& rows = model_.Rows();
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        const auto top = static_cast<int>(index) * kRowHeight - static_cast<int>(scroll_);
        if (top + kRowHeight <= 0 || top >= getHeight())
        {
            continue;
        }
        const auto& row = rows[index];
        const auto alpha = static_cast<float>(model_.OutputAlphaFor(row.id));
        graphics.setColour(shared::ui::kActive.withAlpha(alpha));
        graphics.fillEllipse(static_cast<float>(kHorizontalPadding),
            static_cast<float>(top + (kRowHeight - kLevelMarkerSize) / 2),
            static_cast<float>(kLevelMarkerSize), static_cast<float>(kLevelMarkerSize));
        graphics.setColour(model_.Local(row.id) ? shared::ui::kActive : shared::ui::kText);
        graphics.drawText(juce::String(features::tracks::viewmodels::TracksViewModel::Label(row)),
            kHorizontalPadding + kLevelMarkerSize + 4, top,
            std::max(0, layout.labelWidth - kLevelMarkerSize - 4), kRowHeight,
            juce::Justification::centredLeft, true);
        const int markerTop = top + (kRowHeight - kProcessorMarkerSize) / 2;
        for (std::size_t processor = 0; processor < kProcessors.size(); ++processor)
        {
            const auto found = std::find_if(row.processors.begin(), row.processors.end(),
                [processor](const auto& item) { return item.id == kProcessors[processor]; });
            if (found == row.processors.end())
            {
                continue;
            }
            const auto x = layout.markers + static_cast<int>(processor) * kProcessorSlotWidth;
            const int markerLeft = x + (kProcessorSlotWidth - kProcessorMarkerSize) / 2;
            const bool bypass = model_.Feedback().Bypass(row.id, found->id, found->bypass);
            graphics.setColour(found->active ? shared::ui::kProcessors[processor] : shared::ui::kLines);
            graphics.drawRect(markerLeft, markerTop, kProcessorMarkerSize, kProcessorMarkerSize, 1);
            graphics.setColour(found->marker
                ? bypass ? shared::ui::kDisabled : shared::ui::kProcessors[processor]
                : shared::ui::kBackground);
            graphics.fillRect(markerLeft + 1, markerTop + 1, kProcessorMarkerSize - 2, kProcessorMarkerSize - 2);
        }
        for (const auto& bank : row.banks)
        {
            const auto x = layout.banks + bank.index * shared::ui::kBankCellSize;
            BankCellRenderer::Draw(graphics, { x, top, shared::ui::kBankCellSize, kRowHeight }, model_, row.id, bank);
        }
        graphics.setColour(shared::ui::kLines);
        graphics.drawHorizontalLine(top + kRowHeight, static_cast<float>(layout.banks),
            static_cast<float>(layout.banks + 7 * shared::ui::kBankCellSize));
    }
    graphics.setColour(shared::ui::kLines);
    if (!rows.empty())
    {
        graphics.drawHorizontalLine(-static_cast<int>(scroll_), static_cast<float>(layout.banks),
            static_cast<float>(layout.banks + 7 * shared::ui::kBankCellSize));
    }
    const int gridBottom = rows.empty() ? 0 : std::clamp(
        static_cast<int>(rows.size()) * kRowHeight - static_cast<int>(scroll_) + 1, 0, getHeight());
    for (int bank = 0; bank <= 7; ++bank)
    {
        graphics.drawVerticalLine(layout.banks + bank * shared::ui::kBankCellSize, 0.0F, static_cast<float>(gridBottom));
    }
}

void TrackListControl::SelectAt(const juce::MouseEvent& event, bool dragging)
{
    if (!isEnabled())
    {
        return;
    }
    const auto rowIndex = static_cast<int>(std::floor((event.position.y + static_cast<int>(scroll_)) / kRowHeight));
    if (rowIndex < 0 || static_cast<std::size_t>(rowIndex) >= model_.Rows().size())
    {
        return;
    }
    const auto& row = model_.Rows()[static_cast<std::size_t>(rowIndex)];
    const auto layout = GetLayout();
    const auto x = event.position.x;
    const auto control = !dragging && event.mods.isCtrlDown();
    if (x >= layout.banks && x < layout.banks + 7 * shared::ui::kBankCellSize)
    {
        const auto index = static_cast<int>((x - layout.banks) / shared::ui::kBankCellSize);
        bankInteraction_.Handle(row.id, index, event, dragging);
        repaint();
    }
    else if (!dragging && x >= layout.markers
        && x < layout.markers + static_cast<int>(kProcessors.size()) * kProcessorSlotWidth)
    {
        const auto index = static_cast<std::size_t>((x - layout.markers) / kProcessorSlotWidth);
        const auto found = std::find_if(row.processors.begin(), row.processors.end(),
            [index](const auto& item) { return item.id == kProcessors[index]; });
        if (found != row.processors.end())
        {
            if (!control && !model_.InstanceFocused(row.id))
            {
                model_.ClearSelection();
            }
            const bool bypass = !model_.Feedback().Bypass(row.id, found->id, found->bypass);
            if (control && event.getNumberOfClicks() == 1)
            {
                model_.Feedback().SetBypass(row.id, found->id, bypass);
            }
            emit_({ control ? event.getNumberOfClicks() > 1 ? TrackIntentKind::ResetProcessor :
                TrackIntentKind::SetProcessorBypass : TrackIntentKind::SelectProcessor,
                row.id, model_.FocusedBank(), found->id, bypass });
            repaint();
        }
    }
    else if (!dragging && x < layout.markers)
    {
        model_.Select(row.id, 0);
        emit_({ TrackIntentKind::SelectBank, row.id, 0 });
        repaint();
    }
}

void TrackListControl::mouseDown(const juce::MouseEvent& event)
{
    bankInteraction_.BeginGesture();
    lastY_ = event.position.y;
    SelectAt(event, false);
}

void TrackListControl::mouseDrag(const juce::MouseEvent& event)
{
    if (event.mods.isShiftDown())
    {
        SelectAt(event, true);
    }
    else
    {
        ScrollBy(lastY_ - event.position.y);
        lastY_ = event.position.y;
    }
}

void TrackListControl::ScrollBy(float amount)
{
    const int contentHeight = model_.Rows().empty() ? 0 : static_cast<int>(model_.Rows().size()) * kRowHeight + 1;
    const auto maximum = std::max(0, contentHeight - getHeight());
    scroll_ = std::clamp(scroll_ + amount, 0.0F, static_cast<float>(maximum));
    PositionRowButtons();
    repaint();
}

void TrackListControl::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0F)
    {
        ScrollBy(wheel.deltaY > 0 ? -static_cast<float>(kRowHeight) : static_cast<float>(kRowHeight));
    }
}

void TrackListControl::resized() { ScrollBy(0.0F); }

} // namespace consolidator::vst3::features::tracks::controls
