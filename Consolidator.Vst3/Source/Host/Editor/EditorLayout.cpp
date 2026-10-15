#include "Host/Editor/EditorLayout.h"

#include <algorithm>
#include <cstddef>

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::host::editor
{

int EditorLayout::Width(bool trackListExpanded)
{
    return kSidebarWidth + (trackListExpanded ? kTrackListWidth : 0) + kProcessingWidth;
}

EditorBounds EditorLayout::Calculate(bool trackListExpanded, int titleTextWidth)
{
    EditorBounds bounds;
    const int processingLeft = Width(trackListExpanded) - kProcessingWidth;
    for (std::size_t index = 0; index < bounds.sidebarActions.size(); ++index)
    {
        bounds.sidebarActions[index] = { 8,
            8 + static_cast<int>(index) * (shared::ui::kButtonHeight + shared::ui::kControlGap),
            kSidebarWidth - 16, shared::ui::kButtonHeight };
    }
    const int left = processingLeft + 20;
    const int buttonsWidth = 4 * shared::ui::kBankCellSize;
    const int titleWidth = std::clamp(titleTextWidth + 4, 20, 1060 - buttonsWidth - 12);
    bounds.trackTitle = { left, 4, titleWidth, 36 };
    bounds.trackButtons = { left + titleWidth + 12, 10, buttonsWidth, shared::ui::kBankCellSize };
    bounds.bankManager = { kSidebarWidth, 0, kTrackListWidth, kEditorHeight };
    bounds.equalizer = { left, 56, 1060, 338 };
    bounds.channelStrip = { processingLeft, 406, kProcessingWidth, kEditorHeight - 436 };
    bounds.status = { left, kEditorHeight - 30, 1060, 26 };
    return bounds;
}

} // namespace consolidator::vst3::host::editor
