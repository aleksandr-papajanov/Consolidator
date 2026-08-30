autowatch = 1;
inlets = 1;
outlets = 2;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { BankManagerPresentation } = require("../../Presenters/BankManager/BankManagerPresentation.js");

function processorIdSupportsSolo(processorId) {
    return processorId !== "input" && processorId !== "output";
}

const BankManagerControlOptions = {
    background: [0.08, 0.08, 0.08, 1],
    text: [0.8, 0.8, 0.8, 1],
    actionText: [1, 1, 1, 1],
    focused: [0.35, 0.7, 1, 1],
    remote: [0.55, 0.55, 0.55, 1],
    solo: [0.35, 0.7, 1, 1],
    mute: [0.95, 0.45, 0.35, 1],
    disabled: [0.25, 0.25, 0.25, 1],
    separator: [0.25, 0.25, 0.25, 1],
    rowHeight: 16,
    bankSize: 16,
    bankGap: 0,
    deviceColumnGap: 5,
    processorMarkerSize: 7,
    processorMarkerGap: 2,
    processorMarkerX: 82,
    actionPanelHeight: 18,
    historyPanelHeight: 18,
    outerPadding: 4,
    sectionGap: 3,
    actionGap: 3,
    panelNavigationWidth: 48,
    panelDeviceWidth: 32,
    panelBypassWidth: 16,
    panelSoloWidth: 16,
    panelNavigationGap: 6,
    panelButtonHeight: 32,
    panelButtonGap: 0,
    panelButtonText: [0.85, 0.85, 0.85, 1],
    panelButtonColors: [
        [0.18, 0.48, 0.72, 1],
        [0.72, 0.38, 0.18, 1],
        [0.52, 0.28, 0.68, 1],
        [0.18, 0.58, 0.42, 1],
        [0.68, 0.24, 0.28, 1]
    ],
    historySlotCount: 32,
    fontSize: 11
};

class BankManagerControl
{
    constructor()
    {
        this.presentation = new BankManagerPresentation();
        this.pendingPresentation = null;
        this.scrollPosition = 0;
        this.pointerDown = false;
        this.pointerX = 0;
        this.pointerY = 0;
        this.pointerClickHandled = false;
        this.pointerShift = false;
        this.pointerGroup = false;
        this.dragging = false;
        this.lastY = 0;
    }

    applyPresentation(presentation)
    {
        if (!presentation) {
            return;
        }

        this.presentation = presentation;
        if (!presentation.enabled) {
            this.dragging = false;
        }

        mgraphics.redraw();
    }

    scrollOffset()
    {
        this.scrollPosition = Math.max(
            0,
            Math.min(this.maximumScrollOffset(), this.scrollPosition)
        );

        return this.scrollPosition;
    }

    maximumScrollOffset()
    {
        let rows = this.presentation.rows || [];
        let contentHeight = rows.length * BankManagerControlOptions.rowHeight;
        return Math.max(0, contentHeight - this.contentHeight());
    }

    contentHeight()
    {
        return Math.max(0, this.layoutHeight() -
            BankManagerControlOptions.actionPanelHeight -
            BankManagerControlOptions.historyPanelHeight -
            BankManagerControlOptions.sectionGap * 2);
    }

    actionPanelY()
    {
        return this.contentHeight() + BankManagerControlOptions.sectionGap;
    }

    historyPanelY()
    {
        return this.actionPanelY() +
            BankManagerControlOptions.actionPanelHeight +
            BankManagerControlOptions.sectionGap;
    }

    primaryWidth()
    {
        return Math.max(0, this.layoutWidth() -
            BankManagerControlOptions.panelNavigationWidth -
            BankManagerControlOptions.panelNavigationGap);
    }

    layoutWidth()
    {
        return Math.max(0, mgraphics.size[0] -
            BankManagerControlOptions.outerPadding * 2);
    }

    layoutHeight()
    {
        return Math.max(0, mgraphics.size[1] -
            BankManagerControlOptions.outerPadding * 2);
    }

    bankCount(rows)
    {
        return (rows || []).reduce((count, row) => {
            return Math.max(count, (row.banks || []).length);
        }, 0);
    }

    bankGridX(rows)
    {
        let gridWidth = this.bankCount(rows) * BankManagerControlOptions.bankSize;
        let instanceButtonWidth = BankManagerControlOptions.bankSize * 2;
        return Math.max(0, this.primaryWidth() - gridWidth -
            instanceButtonWidth - BankManagerControlOptions.deviceColumnGap);
    }

    historyLayout()
    {
        let history = this.presentation.history || {};
        let cursor = Number(history.cursor) || 0;
        let entryCount = Number(history.entryCount) || 0;
        let slotSize = BankManagerControlOptions.bankSize;
        let windowStart = Math.max(
            0,
            cursor - Math.floor(BankManagerControlOptions.historySlotCount / 2)
        );
        let cursorSlot = cursor - windowStart;
        let originX = (this.primaryWidth() - slotSize) / 2 -
            cursorSlot * slotSize;
        return {
            cursor: cursor,
            entryCount: entryCount,
            windowStart: windowStart,
            originX: originX,
            slotSize: slotSize
        };
    }

    scrollBy(delta)
    {
        let nextPosition = Math.max(
            0,
            Math.min(
                this.maximumScrollOffset(),
                this.scrollOffset() + delta
            )
        );
        if (nextPosition === this.scrollPosition) {
            return;
        }

        this.scrollPosition = nextPosition;
        mgraphics.redraw();
    }

    emit(name, payload)
    {
        if (payload === undefined) {
            outlet(0, name);
        } else if (payload instanceof Array) {
            outlet(0, [name].concat(payload));
        } else {
            outlet(0, [name, payload]);
        }
    }

    groupLabel(groupId)
    {
        let value = Number(groupId);
        if (!isFinite(value) || value < 0) {
            return "";
        }

        value = Math.floor(value);
        let label = "";
        do {
            label = String.fromCharCode(65 + value % 26) + label;
            value = Math.floor(value / 26) - 1;
        } while (value >= 0);

        return label;
    }

    paintBank(bank, x, y, highlighted)
    {
        if (!bank.visible) {
            return;
        }

        let active = bank.active || bank.selected || highlighted;
        let fallbackColor = bank.system
            ? BankManagerControlOptions.text
            : BankManagerControlOptions.focused;
        let color = bank.color || fallbackColor;
        let alpha = bank.opacity === undefined ? 1 : bank.opacity;
        let displayColor = [
            color[0],
            color[1],
            color[2],
            (color[3] === undefined ? 1 : color[3]) * alpha *
                (bank.active ? 1 : 0.8)
        ];
        let fallbackTextColor = active
            ? BankManagerControlOptions.background
            : displayColor;
        let textColor = bank.textColor || fallbackTextColor;
        let bankHeight = active
            ? BankManagerControlOptions.rowHeight
            : BankManagerControlOptions.bankSize;

        if (active) {
            mgraphics.set_source_rgba.apply(mgraphics, displayColor);
            mgraphics.rectangle(
                x,
                y,
                BankManagerControlOptions.bankSize,
                bankHeight
            );
            mgraphics.fill();
        }

        if (bank.effectActive) {
            mgraphics.set_source_rgba.apply(
                mgraphics,
                active
                    ? BankManagerControlOptions.background
                    : displayColor
            );
            mgraphics.rectangle(x + 1, y + 1, 3, 3);
            mgraphics.fill();
        }

        mgraphics.set_source_rgba.apply(mgraphics, textColor);
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(9);
        if (bank.groupId >= 0) {
            let label = this.groupLabel(bank.groupId);
            let textSize = mgraphics.text_measure(label);
            let fontExtents = mgraphics.font_extents();
            let textX = x + (BankManagerControlOptions.bankSize - textSize[0]) / 2;
            let textY = y + (bankHeight - fontExtents[2]) / 2 + fontExtents[0];
            mgraphics.move_to(textX, textY);
            mgraphics.show_text(label);
        }
    }

    paint()
    {
        let width = this.layoutWidth();
        let height = this.layoutHeight();
        let rows = this.presentation.rows || [];
        let offset = this.scrollOffset();
        let highlightedGroups = {};
        for (let rowIndex = 0; rowIndex < rows.length; rowIndex += 1) {
            for (let bankIndex = 0;
                    bankIndex < rows[rowIndex].banks.length;
                    bankIndex += 1) {
                let bank = rows[rowIndex].banks[bankIndex];
                if ((bank.active || bank.selected) && bank.groupId >= 0) {
                    highlightedGroups[String(bank.groupId)] = true;
                }
            }
        }

        mgraphics.set_source_rgba.apply(
            mgraphics,
            BankManagerControlOptions.background
        );
        mgraphics.rectangle(0, 0, mgraphics.size[0], mgraphics.size[1]);
        mgraphics.fill();
        mgraphics.translate(
            BankManagerControlOptions.outerPadding,
            BankManagerControlOptions.outerPadding
        );
        mgraphics.set_source_rgba.apply(
            mgraphics,
            BankManagerControlOptions.background
        );
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(BankManagerControlOptions.fontSize);

        let contentHeight = this.contentHeight();
        this.paintActions(this.primaryWidth(), this.actionPanelY());
        this.paintHistory(this.primaryWidth(), this.historyPanelY());
        this.paintBankGrid(rows, offset);

        for (let rowIndex = 0; rowIndex < rows.length; rowIndex += 1) {
            let row = rows[rowIndex];
            let y = rowIndex * BankManagerControlOptions.rowHeight - offset;
            if (y + BankManagerControlOptions.rowHeight < 0 ||
                    y + BankManagerControlOptions.rowHeight > contentHeight) {
                continue;
            }

            mgraphics.set_source_rgba.apply(
                mgraphics,
                row.local
                    ? BankManagerControlOptions.focused
                    : BankManagerControlOptions.remote
            );
            mgraphics.set_font_size(BankManagerControlOptions.fontSize);
            mgraphics.move_to(2, y + 12);
            mgraphics.show_text(row.label);

            this.paintProcessorMarkers(row, y);

            for (let bankIndex = 0; bankIndex < row.banks.length; bankIndex += 1) {
                this.paintBank(
                    row.banks[bankIndex],
                    this.bankGridX(rows) + bankIndex * (
                        BankManagerControlOptions.bankSize +
                        BankManagerControlOptions.bankGap
                    ),
                    y,
                    highlightedGroups[String(row.banks[bankIndex].groupId)] === true
                );
            }
            this.paintInstanceButtons(
                row,
                this.bankGridRight(rows) +
                    BankManagerControlOptions.deviceColumnGap,
                y
            );
        }

        this.paintPanelNavigation();
        mgraphics.translate(
            -BankManagerControlOptions.outerPadding,
            -BankManagerControlOptions.outerPadding
        );

    }

    paintPanelNavigation()
    {
        let x = this.primaryWidth() +
            BankManagerControlOptions.panelNavigationGap;
        let labels = ["IN", "SAT", "DYN", "EQ", "OUT"];
        let panelKeys = ["input", "saturator", "compressor", "equalizer", "output"];
        let selected = String(this.presentation.selectedPanel || "").toLowerCase();
        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.background);
        mgraphics.rectangle(x, 0, BankManagerControlOptions.panelNavigationWidth,
            mgraphics.size[1]);
        mgraphics.fill();
        let panelHeight = BankManagerControlOptions.panelButtonHeight;
        let navigationHeight = labels.length * panelHeight;
        let navigationWidth = BankManagerControlOptions.panelNavigationWidth;
        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.separator);
        mgraphics.rectangle(x, 0, navigationWidth, 1);
        mgraphics.fill();
        mgraphics.rectangle(x, navigationHeight - 1, navigationWidth, 1);
        mgraphics.fill();
        mgraphics.rectangle(x, 0, 1, navigationHeight);
        mgraphics.fill();
        mgraphics.rectangle(x + navigationWidth - 1, 0, 1, navigationHeight);
        mgraphics.fill();
        mgraphics.rectangle(x + BankManagerControlOptions.panelDeviceWidth,
            0, 1, navigationHeight);
        mgraphics.fill();
        for (let boundary = 1; boundary < labels.length; boundary += 1) {
            mgraphics.rectangle(x, boundary * panelHeight, navigationWidth, 1);
            mgraphics.fill();
        }
        let controlHalf = panelHeight / 2;
        for (let panelIndex = 0; panelIndex < panelKeys.length; panelIndex += 1) {
            if (!processorIdSupportsSolo(panelKeys[panelIndex])) {
                continue;
            }
            mgraphics.rectangle(
                x + BankManagerControlOptions.panelDeviceWidth,
                panelIndex * panelHeight + controlHalf,
                BankManagerControlOptions.panelBypassWidth,
                1
            );
            mgraphics.fill();
        }
        labels.forEach((label, index) => {
            let y = index * (BankManagerControlOptions.panelButtonHeight +
                BankManagerControlOptions.panelButtonGap);
            let active = panelKeys[index] === selected;
            let processor = (this.presentation.rows || []).filter((row) => row.local)[0];
            let status = processor && (processor.processors || []).filter((item) => {
                return item.processorId === panelKeys[index];
            })[0];
            let color = BankManagerControlOptions.panelButtonColors[index];
            mgraphics.set_source_rgba.apply(mgraphics,
                active ? color : BankManagerControlOptions.background);
            let panelInset = active ? 0 : 1;
            mgraphics.rectangle(x + panelInset, y + panelInset,
                BankManagerControlOptions.panelDeviceWidth - panelInset * 2,
                BankManagerControlOptions.panelButtonHeight - panelInset * 2);
            mgraphics.fill();
            mgraphics.set_source_rgba.apply(mgraphics,
                active ? BankManagerControlOptions.background :
                    color);
            mgraphics.select_font_face("Arial");
            mgraphics.set_font_size(9);
            let textSize = mgraphics.text_measure(label);
            let fontExtents = mgraphics.font_extents();
            mgraphics.move_to(x + (BankManagerControlOptions.panelDeviceWidth -
                textSize[0]) / 2,
                y + (BankManagerControlOptions.panelButtonHeight - fontExtents[2]) / 2 +
                    fontExtents[0]);
            mgraphics.show_text(label);
            let controlX = x + BankManagerControlOptions.panelDeviceWidth;
            let controlHalf = BankManagerControlOptions.panelButtonHeight / 2;
            let supportsSolo = processorIdSupportsSolo(panelKeys[index]);
            let bypassActive = status && status.bypassed;
            let buttons = [
                { label: "B", active: bypassActive, enabled: Boolean(status) },
            ];
            if (supportsSolo) {
                buttons.push({
                    label: "S",
                    active: status && status.soloed,
                    enabled: Boolean(status)
                });
            }
            buttons.forEach((button, buttonIndex) => {
                let buttonHeight = controlHalf;
                let buttonY = y + buttonIndex * buttonHeight;
                mgraphics.set_source_rgba.apply(mgraphics,
                    button.enabled && button.active
                        ? color : BankManagerControlOptions.background);
                let buttonInset = button.active ? 0 : 1;
                mgraphics.rectangle(controlX + buttonInset, buttonY + buttonInset,
                    BankManagerControlOptions.panelBypassWidth - buttonInset * 2,
                    buttonHeight - buttonInset * 2);
                mgraphics.fill();
                mgraphics.set_source_rgba.apply(mgraphics,
                    button.enabled
                        ? button.active ? BankManagerControlOptions.background :
                            BankManagerControlOptions.separator
                        : BankManagerControlOptions.disabled);
                mgraphics.select_font_face("Arial");
                mgraphics.set_font_size(8);
                let buttonTextSize = mgraphics.text_measure(button.label);
                let buttonFontExtents = mgraphics.font_extents();
                mgraphics.move_to(controlX + (BankManagerControlOptions.panelBypassWidth -
                    buttonTextSize[0]) / 2,
                    buttonY + (buttonHeight - buttonFontExtents[2]) / 2 +
                        buttonFontExtents[0]);
                mgraphics.show_text(button.label);
            });
            if (status && status.markerActive) {
                mgraphics.set_source_rgba.apply(mgraphics,
                    active ? BankManagerControlOptions.background : color);
                mgraphics.rectangle(x + 2, y + 2, 3, 3);
                mgraphics.fill();
            }
        });
    }

    paintProcessorMarkers(row, y)
    {
        let processorIds = ["input", "saturator", "compressor", "equalizer", "output"];
        processorIds.forEach((processorId, index) => {
            let processor = (row.processors || []).filter((candidate) => {
                return candidate.processorId === processorId;
            })[0] || { effectActive: false };
            let x = this.processorMarkerX(index);
            let color = BankManagerControlOptions.panelButtonColors[index];
            mgraphics.set_source_rgba.apply(mgraphics,
                processor.effectActive ? color : BankManagerControlOptions.disabled);
            mgraphics.rectangle(x, y + 5,
                BankManagerControlOptions.processorMarkerSize,
                BankManagerControlOptions.processorMarkerSize);
            mgraphics.fill();
        });
    }

    processorMarkerX(index)
    {
        return BankManagerControlOptions.processorMarkerX + index *
            (BankManagerControlOptions.processorMarkerSize +
                BankManagerControlOptions.processorMarkerGap);
    }

    panelAt(x, y)
    {
        let navigationX = this.primaryWidth() +
            BankManagerControlOptions.panelNavigationGap;
        if (x < navigationX || x >= this.layoutWidth() || y < 0 ||
                y >= this.layoutHeight()) {
            return null;
        }
        let index = Math.floor(y / (BankManagerControlOptions.panelButtonHeight +
            BankManagerControlOptions.panelButtonGap));
        return ["input", "saturator", "compressor", "equalizer", "output"][index] || null;
    }

    panelControlAt(x, y)
    {
        let navigationX = this.primaryWidth() +
            BankManagerControlOptions.panelNavigationGap;
        if (x < navigationX || x >= navigationX +
                BankManagerControlOptions.panelNavigationWidth) return null;
        let index = Math.floor(y / (BankManagerControlOptions.panelButtonHeight +
            BankManagerControlOptions.panelButtonGap));
        if (index < 0 || index >= 5) return null;
        let localRow = (this.presentation.rows || []).filter((row) => row.local)[0];
        if (!localRow) return null;
        let processorId = ["input", "saturator", "compressor", "equalizer", "output"][index];
        let supportsSolo = processorIdSupportsSolo(processorId);
        let status = (localRow.processors || []).filter((item) => {
            return item.processorId === processorId;
        })[0];
        let relativeX = x - navigationX;
        let relativeY = y % (BankManagerControlOptions.panelButtonHeight +
            BankManagerControlOptions.panelButtonGap);
        let controlHalf = BankManagerControlOptions.panelButtonHeight / 2;
        if (relativeX >= BankManagerControlOptions.panelDeviceWidth &&
                relativeX < BankManagerControlOptions.panelDeviceWidth +
                    BankManagerControlOptions.panelBypassWidth &&
                relativeY < controlHalf) {
            return { type: "bypass", processorId: processorId,
                value: !(status && status.bypassed) };
        }
        if (supportsSolo &&
                relativeX >= BankManagerControlOptions.panelDeviceWidth &&
                relativeY >= controlHalf) {
            return { type: "solo", processorId: processorId,
                value: !(status && status.soloed) };
        }
        return null;
    }

    paintBankGrid(rows, offset)
    {
        let bankCount = 0;
        for (let rowIndex = 0; rowIndex < rows.length; rowIndex += 1) {
            bankCount = Math.max(bankCount, rows[rowIndex].banks.length);
        }
        if (bankCount === 0) {
            return;
        }

        let gridX = this.bankGridX(rows);
        let gridWidth = bankCount * BankManagerControlOptions.bankSize;
        let gridHeight = rows.length * BankManagerControlOptions.rowHeight;
        let gridTop = Math.max(0, -offset);
        let gridBottom = Math.min(this.contentHeight(), gridHeight - offset);
        if (gridBottom <= gridTop) {
            return;
        }

        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.separator);

        for (let bankIndex = 0; bankIndex <= bankCount; bankIndex += 1) {
            mgraphics.rectangle(
                gridX + bankIndex * BankManagerControlOptions.bankSize,
                gridTop,
                1,
                gridBottom - gridTop
            );
            mgraphics.fill();
        }

        for (let rowIndex = 0; rowIndex <= rows.length; rowIndex += 1) {
            let y = rowIndex * BankManagerControlOptions.rowHeight - offset;
            if (y < 0 || y > this.contentHeight()) {
                continue;
            }

            mgraphics.rectangle(gridX, y, gridWidth, 1);
            mgraphics.fill();
        }

        let buttonsX = this.bankGridRight(rows) +
            BankManagerControlOptions.deviceColumnGap;
        for (let buttonIndex = 0; buttonIndex <= 2; buttonIndex += 1) {
            mgraphics.rectangle(
                buttonsX + buttonIndex * BankManagerControlOptions.bankSize,
                gridTop,
                1,
                gridBottom - gridTop
            );
            mgraphics.fill();
        }
        for (let rowIndex = 0; rowIndex <= rows.length; rowIndex += 1) {
            let y = rowIndex * BankManagerControlOptions.rowHeight - offset;
            if (y < 0 || y > this.contentHeight()) {
                continue;
            }

            mgraphics.rectangle(
                buttonsX,
                y,
                2 * BankManagerControlOptions.bankSize,
                1
            );
            mgraphics.fill();
        }
    }

    bankGridRight(rows)
    {
        let bankCount = rows.reduce((count, row) => {
            return Math.max(count, row.banks.length);
        }, 0);
        return this.bankGridX(rows) +
            bankCount * BankManagerControlOptions.bankSize;
    }

    paintInstanceButtons(row, x, y)
    {
        [
            { label: "S", active: Boolean(row.solo), color: BankManagerControlOptions.solo },
            { label: "M", active: Boolean(row.mute), color: BankManagerControlOptions.mute }
        ].forEach((button, index) => {
            let buttonX = x + index * BankManagerControlOptions.bankSize;
            if (button.active) {
                mgraphics.set_source_rgba.apply(
                    mgraphics,
                    button.color
                );
                mgraphics.rectangle(
                    buttonX,
                    y,
                    BankManagerControlOptions.bankSize,
                    BankManagerControlOptions.rowHeight
                );
                mgraphics.fill();
            }
            mgraphics.set_source_rgba.apply(
                mgraphics,
                button.active
                    ? BankManagerControlOptions.background
                    : BankManagerControlOptions.separator
            );
            mgraphics.select_font_face("Arial");
            mgraphics.set_font_size(9);
            let textSize = mgraphics.text_measure(button.label);
            mgraphics.move_to(
                buttonX + (BankManagerControlOptions.bankSize - textSize[0]) / 2,
                y + 12
            );
            mgraphics.show_text(button.label);
        });
    }

    paintActions(width, contentHeight)
    {
        let group = this.presentation.groupAction || {};
        let ungroup = this.presentation.ungroupAction || {};
        let clear = this.presentation.clearAction || {};
        let actions = [
            { label: "Group", action: group },
            { label: "Ungroup", action: ungroup },
            { label: clear.armed ? "Sure?" : "Clear", action: clear }
        ];
        let buttonWidth = (width - BankManagerControlOptions.actionGap *
            (actions.length - 1)) / actions.length;
        let y = contentHeight;
        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.background);
        mgraphics.rectangle(0, y, width, BankManagerControlOptions.actionPanelHeight);
        mgraphics.fill();
        actions.forEach((entry, index) => {
            let x = Math.round(index * (buttonWidth +
                BankManagerControlOptions.actionGap));
            let right = Math.round(x + buttonWidth);
            let actualWidth = right - x;
            let borderColor = BankManagerControlOptions.separator;
            let textColor = entry.action.enabled
                ? BankManagerControlOptions.actionText
                : BankManagerControlOptions.separator;
            mgraphics.set_source_rgba.apply(
                mgraphics,
                BankManagerControlOptions.background
            );
            mgraphics.rectangle(x, y, actualWidth,
                BankManagerControlOptions.actionPanelHeight);
            mgraphics.fill();
            mgraphics.set_source_rgba.apply(mgraphics, borderColor);
            mgraphics.rectangle(x, y, actualWidth, 1);
            mgraphics.fill();
            mgraphics.rectangle(x, y + BankManagerControlOptions.actionPanelHeight - 1,
                actualWidth, 1);
            mgraphics.fill();
            mgraphics.rectangle(x, y, 1,
                BankManagerControlOptions.actionPanelHeight);
            mgraphics.fill();
            mgraphics.rectangle(right - 1, y, 1,
                BankManagerControlOptions.actionPanelHeight);
            mgraphics.fill();
            mgraphics.set_source_rgba.apply(mgraphics, textColor);
            mgraphics.select_font_face("Arial");
            mgraphics.set_font_size(9);
            let textSize = mgraphics.text_measure(entry.label);
            mgraphics.move_to(x + (actualWidth - textSize[0]) / 2, y + 12);
            mgraphics.show_text(entry.label);
        });
    }

    paintHistory(width, y)
    {
        let history = this.presentation.history || {};
        let layout = this.historyLayout();
        let cursor = layout.cursor;
        let entryCount = layout.entryCount;
        let cellSize = layout.slotSize;
        let gridX = layout.originX;
        let historyHeight = BankManagerControlOptions.historyPanelHeight;

        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.background);
        mgraphics.rectangle(0, y, width, historyHeight);
        mgraphics.fill();
        for (let slot = 0;
                slot < BankManagerControlOptions.historySlotCount;
                slot += 1) {
            let x = gridX + slot * cellSize;
            let historyIndex = layout.windowStart + slot;
            let available = historyIndex <= entryCount;
            if (!available) {
                continue;
            }
            let current = historyIndex === cursor;
            let fillColor = current
                ? BankManagerControlOptions.focused
                : null;
            if (fillColor) {
                mgraphics.set_source_rgba.apply(mgraphics, fillColor);
                mgraphics.rectangle(x, y, cellSize, historyHeight);
                mgraphics.fill();
            }
            mgraphics.set_source_rgba.apply(mgraphics,
                BankManagerControlOptions.separator);
            mgraphics.rectangle(x, y, 1, historyHeight);
            mgraphics.fill();
            mgraphics.rectangle(x, y, cellSize, 1);
            mgraphics.fill();
            mgraphics.rectangle(x, y + historyHeight - 1, cellSize, 1);
            mgraphics.fill();
            mgraphics.set_source_rgba.apply(mgraphics,
                current
                    ? BankManagerControlOptions.background
                    : BankManagerControlOptions.separator);
            mgraphics.set_font_size(8);
            let label = String(historyIndex);
            let textSize = mgraphics.text_measure(label);
            mgraphics.move_to(x + (cellSize - textSize[0]) / 2, y + 12);
            mgraphics.show_text(label);
        }

        let lastAvailableSlot = Math.min(
            BankManagerControlOptions.historySlotCount - 1,
            entryCount - layout.windowStart
        );
        if (lastAvailableSlot >= 0) {
            let rightX = gridX + (lastAvailableSlot + 1) * cellSize - 1;
            mgraphics.set_source_rgba.apply(mgraphics,
                BankManagerControlOptions.separator);
            mgraphics.rectangle(rightX, y, 1, historyHeight);
            mgraphics.fill();
        }
    }

    historyAt(x, y)
    {
        let historyY = this.historyPanelY();
        if (y < historyY || y >= historyY +
                BankManagerControlOptions.historyPanelHeight) {
            return -1;
        }
        let layout = this.historyLayout();
        let slot = Math.floor((x - layout.originX) / layout.slotSize);
        let historyIndex = layout.windowStart + slot;
        return slot >= 0 && slot < BankManagerControlOptions.historySlotCount &&
            historyIndex <= layout.entryCount ? historyIndex : -1;
    }

    rowAt(y)
    {
        return Math.floor(
            (y + this.scrollOffset()) / BankManagerControlOptions.rowHeight
        );
    }

    bankAt(row, x)
    {
        let index = Math.floor(
            (x - this.bankGridX(this.presentation.rows || [])) /
            (BankManagerControlOptions.bankSize + BankManagerControlOptions.bankGap)
        );

        return index >= 0 && row && row.banks[index] ? index : -1;
    }

    selectAt(x, y, extendSelection, groupControl)
    {
        if (x < BankManagerControlOptions.outerPadding ||
                y < BankManagerControlOptions.outerPadding ||
                x >= mgraphics.size[0] - BankManagerControlOptions.outerPadding ||
                y >= mgraphics.size[1] - BankManagerControlOptions.outerPadding) {
            return;
        }
        x -= BankManagerControlOptions.outerPadding;
        y -= BankManagerControlOptions.outerPadding;
        if (!this.presentation.enabled) {
            return;
        }

        let panel = this.panelAt(x, y);
        let panelControl = this.panelControlAt(x, y);
        if (panelControl) {
            if (panelControl.type === "bypass") {
                this.emit("processorBypassChanged", [
                    (this.presentation.rows || []).filter((row) => row.local)[0].instanceId,
                    panelControl.processorId,
                    panelControl.value ? 1 : 0,
                    groupControl ? 1 : 0
                ]);
            } else if (panelControl.processorId !== "input" &&
                    panelControl.processorId !== "output") {
                this.emit("processorSoloChanged", [
                    (this.presentation.rows || []).filter((row) => row.local)[0].instanceId,
                    panelControl.processorId,
                    panelControl.value ? 1 : 0,
                    extendSelection ? 1 : 0,
                    groupControl ? 1 : 0
                ]);
            }
            return;
        }
        if (panel) {
            this.emit("panelSelected", [panel]);
            return;
        }

        let historySlot = this.historyAt(x, y);
        if (historySlot >= 0) {
            this.emit("historySelected", [historySlot]);
            return;
        }

        if (y >= this.actionPanelY() &&
                y < this.actionPanelY() + BankManagerControlOptions.actionPanelHeight) {
            let actionWidth = (this.primaryWidth() - BankManagerControlOptions.actionGap * 2) / 3;
            let actionStep = actionWidth + BankManagerControlOptions.actionGap;
            let buttonIndex = Math.floor(x / actionStep);
            if (buttonIndex < 0 || buttonIndex >= 3 ||
                    x >= buttonIndex * actionStep + actionWidth) {
                return;
            }
            let actions = [
                this.presentation.groupAction,
                this.presentation.ungroupAction,
                this.presentation.clearAction
            ];
            let action = actions[buttonIndex];
            if (action && action.enabled) {
                this.emit([
                    "groupRequested",
                    "ungroupRequested",
                    "clearRequested"
                ][buttonIndex]);
            }
            return;
        }

        let rows = this.presentation.rows || [];
        let rowIndex = this.rowAt(y);
        let row = rows[rowIndex];
        if (!row) {
            return;
        }

        let instanceButtonsX = this.bankGridRight(rows) +
            BankManagerControlOptions.deviceColumnGap;
        if (x >= instanceButtonsX &&
                x < instanceButtonsX + BankManagerControlOptions.bankSize * 2) {
            let buttonIndex = Math.floor(
                (x - instanceButtonsX) / BankManagerControlOptions.bankSize
            );
            if (buttonIndex === 0) {
                this.emit("instanceSoloChanged", [
                    row.instanceId,
                    row.solo ? 0 : 1,
                    extendSelection ? 1 : 0,
                    groupControl ? 1 : 0
                ]);
            } else {
                this.emit("instanceMuteChanged", [
                    row.instanceId,
                    row.mute ? 0 : 1,
                    groupControl ? 1 : 0
                ]);
            }
            return;
        }

        let bankIndex = this.bankAt(row, x);
        if (bankIndex >= 0) {
            let bank = row.banks[bankIndex];
            if (bank.visible && bank.enabled) {
                this.emit("bankSelected", [
                    row.instanceId,
                    bank.bankId,
                    extendSelection ? 1 : 0
                ]);
            }
        } else {
            this.emit("rowSelected", [row.instanceId]);
        }
    }

    beginGesture(y)
    {
        if (!this.presentation.enabled) {
            return;
        }

        this.dragging = true;
        this.lastY = y;
        this.emit("gestureBegan");
    }

    beginPointer(x, y, shift, groupControl)
    {
        if (!this.presentation.enabled) {
            return;
        }

        this.pointerDown = true;
        this.pointerClickHandled = false;
        this.pointerX = x;
        this.pointerY = y;
        this.pointerShift = Number(shift) !== 0;
        this.pointerGroup = Number(groupControl) !== 0;
        this.dragging = false;
    }

    movePointer(x, y)
    {
        if (!this.pointerDown) {
            return;
        }

        if (!this.dragging) {
            let distance = Math.sqrt(
                Math.pow(x - this.pointerX, 2) + Math.pow(y - this.pointerY, 2)
            );
            if (distance < 4) {
                return;
            }

            this.beginGesture(this.pointerY);
        }

        this.drag(y);
    }

    endPointer(x, y)
    {
        if (!this.pointerDown) {
            return;
        }

        if (this.dragging) {
            this.endGesture();
        } else if (!this.pointerClickHandled) {
            this.selectAt(
                x,
                y,
                this.pointerShift,
                this.pointerGroup
            );
        }

        this.pointerDown = false;
        this.pointerShift = false;
        this.pointerGroup = false;
        this.pointerClickHandled = false;
    }

    cancelPointer()
    {
        if (this.dragging) {
            this.endGesture();
        }

        this.pointerDown = false;
        this.pointerShift = false;
        this.pointerGroup = false;
        this.pointerClickHandled = false;
    }

    drag(y)
    {
        if (!this.dragging) {
            return;
        }

        this.scrollPosition = Math.max(
            0,
            Math.min(
                this.maximumScrollOffset(),
                this.scrollOffset() + this.lastY - y
            )
        );
        this.lastY = y;
        mgraphics.redraw();
    }

    endGesture()
    {
        if (!this.dragging) {
            return;
        }

        this.dragging = false;
        this.emit("gestureEnded");
    }

    colorFromArguments(hasColor,
    red,
    green,
    blue,
    alpha)
    {
        if (Number(hasColor) === 0) {
            return null;
        }

        return [
            Number(red),
            Number(green),
            Number(blue),
            Number(alpha)
        ];
    }

    beginPresentation(enabled)
    {
        this.pendingPresentation = new BankManagerPresentation();
        this.pendingPresentation.enabled = Number(enabled) !== 0;
    }

    addRow(index,
    instanceId,
    label,
    local,
    solo,
    mute)
    {
        if (!this.pendingPresentation) {
            return;
        }

        let presentation = this.pendingPresentation;
        presentation.rows[Number(index)] = {
            instanceId: instanceId,
            label: String(label),
            local: Number(local) !== 0,
            solo: Number(solo) !== 0,
            mute: Number(mute) !== 0,
            processors: [],
            banks: []
        };
    }

    addProcessor(rowIndex,
    processorId,
    effectActive,
    markerActive,
    bypassed,
    soloed)
    {
        if (!this.pendingPresentation) {
            return;
        }

        let row = this.pendingPresentation.rows[Number(rowIndex)];
        if (!row) return;
        row.processors.push({
            processorId: String(processorId),
            effectActive: Number(effectActive) !== 0,
            markerActive: Number(markerActive) !== 0,
            bypassed: Number(bypassed) !== 0,
            soloed: Number(soloed) !== 0
        });
    }

    addBank(rowIndex,
    bankId,
    label,
    system,
    visible,
    enabled,
    active,
    selected,
    opacity,
    groupId,
    effectActive,
    hasColor,
    red,
    green,
    blue,
    alpha,
    hasTextColor,
    textRed,
    textGreen,
    textBlue,
    textAlpha)
    {
        if (!this.pendingPresentation) {
            return;
        }

        let targetRow = this.pendingPresentation.rows[Number(rowIndex)];
        if (!targetRow) {
            return;
        }

        targetRow.banks.push({
            bankId: bankId,
            label: String(label),
            system: Number(system) !== 0,
            visible: Number(visible) !== 0,
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0,
            selected: Number(selected) !== 0,
            opacity: Number(opacity),
            groupId: Number(groupId),
            effectActive: Number(effectActive) !== 0,
            color: this.colorFromArguments(
                hasColor,
                red,
                green,
                blue,
                alpha
            ),
            textColor: this.colorFromArguments(
                hasTextColor,
                textRed,
                textGreen,
                textBlue,
                textAlpha
            )
        });
    }

    setGroupAction(enabled, active)
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.pendingPresentation.groupAction = {
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0
        };
    }

    setUngroupAction(enabled, active)
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.pendingPresentation.ungroupAction = {
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0
        };
    }

    setClearAction(enabled, armed)
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.pendingPresentation.clearAction = {
            enabled: Number(enabled) !== 0,
            armed: Number(armed) !== 0
        };
    }

    setHistory(cursor, entryCount, canUndo, canRedo)
    {
        if (!this.pendingPresentation) {
            return;
        }
        this.pendingPresentation.history = {
            cursor: Number(cursor),
            entryCount: Number(entryCount),
            canUndo: Number(canUndo) !== 0,
            canRedo: Number(canRedo) !== 0
        };
    }

    setSelectedPanel(panel)
    {
        if (!this.pendingPresentation) {
            return;
        }
        this.pendingPresentation.selectedPanel = String(panel);
    }

    patchHistory(cursor, entryCount, canUndo, canRedo)
    {
        this.presentation.history = {
            cursor: Number(cursor),
            entryCount: Number(entryCount),
            canUndo: Number(canUndo) !== 0,
            canRedo: Number(canRedo) !== 0
        };
    }

    endPresentation()
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.applyPresentation(this.pendingPresentation);
        this.pendingPresentation = null;
    }

    beginPresentationPatch(enabled)
    {
        this.presentation.enabled = Number(enabled) !== 0;
    }

    patchRow(index,
    instanceId,
    label,
    local,
    solo,
    mute)
    {
        let rowIndex = Number(index);
        let row = this.presentation.rows[rowIndex];
        if (!row) {
            row = { banks: [] };
            this.presentation.rows[rowIndex] = row;
        }
        row.instanceId = instanceId;
        row.label = String(label);
        row.local = Number(local) !== 0;
        row.solo = Number(solo) !== 0;
        row.mute = Number(mute) !== 0;
        if (!row.processors) row.processors = [];
    }

    patchProcessor(rowIndex,
    processorId,
    effectActive,
    markerActive,
    bypassed,
    soloed)
    {
        let row = this.presentation.rows[Number(rowIndex)];
        if (!row) return;
        if (!row.processors) row.processors = [];
        let processor = row.processors.filter((candidate) => {
            return candidate.processorId === String(processorId);
        })[0];
        if (!processor) {
            processor = { processorId: String(processorId) };
            row.processors.push(processor);
        }
        processor.effectActive = Number(effectActive) !== 0;
        processor.markerActive = Number(markerActive) !== 0;
        processor.bypassed = Number(bypassed) !== 0;
        processor.soloed = Number(soloed) !== 0;
    }

    removeRow(index)
    {
        let rowIndex = Number(index);
        if (rowIndex >= 0 && rowIndex < this.presentation.rows.length) {
            this.presentation.rows.splice(rowIndex, 1);
        }
    }

    patchBank(...args)
    {
        let rowIndex = Number(args[0]);
        let bankId = Number(args[1]);
        let row = this.presentation.rows[rowIndex];
        if (!row) return;

        let bank = {
            bankId: args[1],
            label: String(args[2]),
            system: Number(args[3]) !== 0,
            visible: Number(args[4]) !== 0,
            enabled: Number(args[5]) !== 0,
            active: Number(args[6]) !== 0,
            selected: Number(args[7]) !== 0,
            opacity: Number(args[8]),
            groupId: Number(args[9]),
            effectActive: Number(args[10]) !== 0,
            color: this.colorFromArguments.apply(this, args.slice(11, 16)),
            textColor: this.colorFromArguments.apply(this, args.slice(16, 21))
        };
        for (let index = 0; index < row.banks.length; index += 1) {
            if (Number(row.banks[index].bankId) === bankId) {
                row.banks[index] = bank;
                return;
            }
        }
        row.banks.push(bank);
    }

    patchGroupAction(enabled, active)
    {
        this.presentation.groupAction = {
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0
        };
    }

    patchUngroupAction(enabled, active)
    {
        this.presentation.ungroupAction = {
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0
        };
    }

    patchClearAction(enabled, armed)
    {
        this.presentation.clearAction = {
            enabled: Number(enabled) !== 0,
            armed: Number(armed) !== 0
        };
    }

    endPresentationPatch()
    {
        this.scrollOffset();
        mgraphics.redraw();
    }
}

function presentation_begin(enabled) {
    bankManagerControl.beginPresentation(enabled);
}

function row(...args) {
    bankManagerControl.addRow(...args);
}

function processor(...args) {
    bankManagerControl.addProcessor(...args);
}

function bank(...args) {
    bankManagerControl.addBank(...args);
}

function group_action(enabled, active) {
    bankManagerControl.setGroupAction(enabled, active);
}

function ungroup_action(enabled, active) {
    bankManagerControl.setUngroupAction(enabled, active);
}

function clear_action(enabled, armed) {
    bankManagerControl.setClearAction(enabled, armed);
}

function history(cursor, entryCount, canUndo, canRedo) {
    bankManagerControl.setHistory(cursor, entryCount, canUndo, canRedo);
}

function selected_panel(panel) {
    bankManagerControl.setSelectedPanel(panel);
    outlet(1, panel);
}

function presentation_end() {
    bankManagerControl.endPresentation();
}

function presentation_patch_begin(enabled) {
    bankManagerControl.beginPresentationPatch(enabled);
}

function row_patch(...args) {
    bankManagerControl.patchRow(...args);
}

function processor_patch(...args) {
    bankManagerControl.patchProcessor(...args);
}

function row_remove(index) {
    bankManagerControl.removeRow(index);
}

function bank_patch(...args) {
    bankManagerControl.patchBank(...args);
}

function group_action_patch(enabled, active) {
    bankManagerControl.patchGroupAction(enabled, active);
}

function ungroup_action_patch(enabled, active) {
    bankManagerControl.patchUngroupAction(enabled, active);
}

function clear_action_patch(enabled, armed) {
    bankManagerControl.patchClearAction(enabled, armed);
}

function history_patch(cursor, entryCount, canUndo, canRedo) {
    bankManagerControl.patchHistory(cursor, entryCount, canUndo, canRedo);
}

function presentation_patch_end() {
    bankManagerControl.endPresentationPatch();
}

function paint() {
    bankManagerControl.paint();
}

function onresize() {
    mgraphics.redraw();
}

function onclick(x, y, button, modifier1, shift, caps, option, modifier2,
    pointerevent) {
    let modifier1Value = Number(modifier1);
    let modifier2Value = Number(modifier2);
    let pointerControl = Boolean(pointerevent &&
        (pointerevent.ctrlKey || pointerevent.metaKey));
    let groupControl = pointerControl ||
        (isFinite(modifier1Value) && modifier1Value !== 0) ||
        (isFinite(modifier2Value) && modifier2Value !== 0);
    bankManagerControl.beginPointer(x, y, shift, groupControl);
    bankManagerControl.selectAt(x, y, shift, groupControl);
    bankManagerControl.pointerClickHandled = true;
}

function ondrag(x, y, button) {
    if (button === 0) {
        bankManagerControl.endPointer(x, y);
    } else {
        bankManagerControl.movePointer(x, y);
    }
}

function onidleout() {
    bankManagerControl.cancelPointer();
}

function onwheel(x, y, delta) {
    if (!bankManagerControl.presentation.enabled) {
        return;
    }

    let wheelDelta = Number(delta);
    if (!isFinite(wheelDelta)) {
        return;
    }

    bankManagerControl.scrollBy(
        -wheelDelta * BankManagerControlOptions.rowHeight
    );
}

const bankManagerControl = new BankManagerControl();
