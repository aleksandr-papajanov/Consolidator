autowatch = 1;
inlets = 1;
outlets = 2;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { BankManagerPresentation } = require("../../Presenters/BankManager/BankManagerPresentation.js");
const { DoubleClickTracker } = require("../DoubleClickTracker.js");
const { UiColors } = require("../../Theme/UiColors.js");
const BankManagerControlOptions = {
    background: UiColors.base.background,
    text: UiColors.base.text,
    focused: UiColors.controls.active,
    remote: UiColors.base.text,
    solo: UiColors.devices.solo,
    mute: UiColors.devices.mute,
    disabled: UiColors.base.disabledText,
    separator: UiColors.base.lines,
    rowHeight: 16,
    bankSize: 16,
    bankGap: 0,
    columnGap: 5,
    deviceColumnGap: 5,
    actionColumnWidth: 64,
    actionButtonHeight: 16,
    historyGroupGap: 8,
    outerPadding: 4,
    sectionGap: 3,
    actionGap: 3,
    actionFlashDurationMs: 180,
    deviceColors: UiColors.devices.processors,
    processorIds: ["input", "saturator", "compressor", "equalizer", "polish", "output"],
    processorMarkerIds: ["input", "saturator", "compressor", "equalizer", "polish", "output"],
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
        this.dragging = false;
        this.lastY = 0;
        this.actionFlash = {};
        this.actionFlashTasks = {};
        this.doubleClick = new DoubleClickTracker();
        this.bypassOverrides = {};
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

    flashAction(actionId)
    {
        let key = String(actionId);
        let previous = this.actionFlashTasks[key];
        if (previous) {
            previous.cancel();
        }
        this.actionFlash[key] = true;
        mgraphics.redraw();
        let task = new Task(() => {
            delete this.actionFlash[key];
            delete this.actionFlashTasks[key];
            mgraphics.redraw();
        }, this);
        this.actionFlashTasks[key] = task;
        task.schedule(BankManagerControlOptions.actionFlashDurationMs);
    }

    bypassKey(instanceId, itemId)
    {
        return String(instanceId) + ":" + String(itemId);
    }

    bypassValue(key, value)
    {
        return this.bypassOverrides[key] === undefined
            ? Boolean(value)
            : this.bypassOverrides[key];
    }

    setBypassOverride(key, value)
    {
        this.bypassOverrides[key] = Boolean(value);
    }

    confirmBypassOverride(key, value)
    {
        if (this.bypassOverrides[key] === Boolean(value)) {
            delete this.bypassOverrides[key];
        }
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
        return this.layoutHeight();
    }

    primaryWidth()
    {
        return this.layoutWidth();
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
        return this.markerGridX(rows) + this.markerWidth() +
            BankManagerControlOptions.columnGap;
    }

    markerGridX(rows)
    {
        let gridWidth = this.bankCount(rows) * BankManagerControlOptions.bankSize;
        let chainWidth = this.markerWidth() + BankManagerControlOptions.columnGap +
            gridWidth + BankManagerControlOptions.deviceColumnGap +
            this.instanceButtonWidth();
        // The label is flexible; the complete control chain is kept right-aligned when it fits.
        return Math.max(0, this.actionsColumnX() - BankManagerControlOptions.columnGap -
            chainWidth);
    }

    markerWidth()
    {
        return BankManagerControlOptions.processorMarkerIds.length *
            BankManagerControlOptions.bankSize;
    }

    instanceButtonWidth()
    {
        return BankManagerControlOptions.bankSize * 4;
    }

    labelWidth(rows)
    {
        return Math.max(0, this.markerGridX(rows) - BankManagerControlOptions.columnGap);
    }

    actionsColumnX()
    {
        return Math.max(0, this.primaryWidth() - BankManagerControlOptions.actionColumnWidth);
    }

    actionGroupHeight()
    {
        let actionCount = 4;
        return actionCount * BankManagerControlOptions.actionButtonHeight +
            (actionCount - 1) * BankManagerControlOptions.actionGap;
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
        this.debug("emit name=" + name + " payload=" + JSON.stringify(payload));
        if (payload === undefined) {
            outlet(0, name);
        } else if (payload instanceof Array) {
            outlet(0, [name].concat(payload));
        } else {
            outlet(0, [name, payload]);
        }
    }

    debug(message)
    {
        if (typeof post === "function") {
            post("[Consolidator][TrackName] BankManagerControl " + message + "\n");
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

    isGroupedBank(bank)
    {
        if (!bank || bank.groupId === undefined || bank.groupId === null) {
            return false;
        }
        let groupId = Number(bank.groupId);
        return isFinite(groupId) && groupId >= 0;
    }

    paintBank(bank, x, y, highlighted, instanceId)
    {
        if (!bank.visible) {
            return;
        }

        let active = bank.active || bank.selected || highlighted;
        let bankBypassed = this.bypassValue(
            this.bypassKey(instanceId, bank.bankId),
            bank.bypassed
        );
        let fallbackColor = bank.system
            ? BankManagerControlOptions.text
            : BankManagerControlOptions.focused;
        let color = bank.selected
            ? BankManagerControlOptions.deviceColors.equalizer
            : bank.color || fallbackColor;
        let alpha = bank.opacity === undefined ? 1 : bank.opacity;
        let displayColor = [
            color[0],
            color[1],
            color[2],
            (color[3] === undefined ? 1 : color[3]) * alpha *
                (bank.active ? 1 : 0.8)
        ];
        let selectionColor = bank.effectActive
            ? displayColor
            : BankManagerControlOptions.separator;
        let fallbackTextColor = active
            ? bank.effectActive
                ? BankManagerControlOptions.background
                : BankManagerControlOptions.disabled
            : displayColor;
        let textColor = bank.textColor || fallbackTextColor;
        let bankHeight = active
            ? BankManagerControlOptions.rowHeight
            : BankManagerControlOptions.bankSize;

        if (active) {
            mgraphics.set_source_rgba.apply(mgraphics, selectionColor);
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
                BankManagerControlOptions.deviceColors.equalizer
            );
            mgraphics.rectangle(x + 1, y + 1, 3, 3);
            mgraphics.fill();
        }

        if (bank.active && bankBypassed) {
            this.paintBypassIndicator(x, y);
        }

        mgraphics.set_source_rgba.apply(mgraphics, textColor);
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(9);
        if (this.isGroupedBank(bank)) {
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
        let groupScope = Boolean((this.presentation.scopeAction || {}).active);
        let highlightedGroups = {};
        for (let rowIndex = 0; rowIndex < rows.length; rowIndex += 1) {
            for (let bankIndex = 0;
                    bankIndex < rows[rowIndex].banks.length;
                    bankIndex += 1) {
                let bank = rows[rowIndex].banks[bankIndex];
                if (groupScope && (bank.active || bank.selected) &&
                        this.isGroupedBank(bank)) {
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
        this.paintActions(
            this.actionsColumnX(),
            this.contentHeight()
        );
        this.paintBankGrid(rows, offset);
        this.paintProcessorMarkerGrid(rows, offset);

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
            let label = String(row.label || "");
            let labelWidth = this.labelWidth(rows) - 2;
            while (label.length > 0 &&
                    mgraphics.text_measure(label)[0] > labelWidth) {
                label = label.substring(0, label.length - 1);
            }
            mgraphics.move_to(2, y + 12);
            mgraphics.show_text(label);

            this.paintProcessorMarkers(row, y);
            for (let bankIndex = 0; bankIndex < row.banks.length; bankIndex += 1) {
                this.paintBank(
                    row.banks[bankIndex],
                    this.bankGridX(rows) + bankIndex * (
                        BankManagerControlOptions.bankSize +
                        BankManagerControlOptions.bankGap
                    ),
                    y,
                    groupScope
                        ? highlightedGroups[String(row.banks[bankIndex].groupId)] === true
                        : Boolean(row.banks[bankIndex].active),
                    row.instanceId
                );
            }
            this.paintInstanceButtons(
                row,
                this.bankGridRight(rows) +
                    BankManagerControlOptions.deviceColumnGap,
                y
            );
        }

        mgraphics.translate(
            -BankManagerControlOptions.outerPadding,
            -BankManagerControlOptions.outerPadding
        );

    }

    paintProcessorMarkers(row, y)
    {
        BankManagerControlOptions.processorMarkerIds.forEach((processorId, index) => {
            let processor = (row.processors || []).filter((candidate) => {
                return candidate.processorId === processorId;
            })[0] || { markerActive: false };
            let x = this.markerGridX(this.presentation.rows || []) +
                index * BankManagerControlOptions.bankSize;
            let color = BankManagerControlOptions.deviceColors[processorId];
            let selectedInstance = (row.banks || []).some((bank) => bank.active);
            let selected = selectedInstance &&
                String(this.presentation.selectedPanel || "").toLowerCase() === processorId;
            if (selected) {
                let selectionColor = processor.effectActive
                    ? color
                    : BankManagerControlOptions.separator;
                mgraphics.set_source_rgba.apply(mgraphics, selectionColor);
                mgraphics.rectangle(
                    x,
                    y,
                    BankManagerControlOptions.bankSize,
                    BankManagerControlOptions.bankSize
                );
                mgraphics.fill();
            }
            if (this.bypassValue(
                    this.bypassKey(row.instanceId, processorId),
                    processor.bypassed)) {
                this.paintBypassIndicator(x, y);
            }
            if (!processor.markerActive) {
                return;
            }
            mgraphics.set_source_rgba.apply(mgraphics,
                color);
            mgraphics.rectangle(x + 1, y + 1, 3, 3);
            mgraphics.fill();
        });
    }

    paintBypassIndicator(x, y)
    {
        mgraphics.set_source_rgba.apply(mgraphics, UiColors.devices.mute);
        mgraphics.set_line_width(1);
        mgraphics.move_to(x + 3, y + 3);
        mgraphics.line_to(x + BankManagerControlOptions.bankSize - 3,
            y + BankManagerControlOptions.bankSize - 3);
        mgraphics.stroke();
        mgraphics.move_to(x + BankManagerControlOptions.bankSize - 3, y + 3);
        mgraphics.line_to(x + 3, y + BankManagerControlOptions.bankSize - 3);
        mgraphics.stroke();
    }

    paintProcessorMarkerGrid(rows, offset)
    {
        if (rows.length === 0) {
            return;
        }

        let gridX = this.markerGridX(rows);
        let gridWidth = this.markerWidth();
        let gridHeight = rows.length * BankManagerControlOptions.rowHeight;
        let gridTop = Math.max(0, -offset);
        let gridBottom = Math.min(this.contentHeight(), gridHeight - offset);
        if (gridBottom <= gridTop) {
            return;
        }

        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.separator);
        for (let markerIndex = 0; markerIndex <=
                BankManagerControlOptions.processorMarkerIds.length; markerIndex += 1) {
            mgraphics.rectangle(
                gridX + markerIndex * BankManagerControlOptions.bankSize,
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
    }

    paintBankGrid(rows, offset)
    {
        if (this.bankCount(rows) === 0) {
            return;
        }

        let gridX = this.bankGridX(rows);
        let gridWidth = this.bankCount(rows) * BankManagerControlOptions.bankSize;
        let gridHeight = rows.length * BankManagerControlOptions.rowHeight;
        let gridTop = Math.max(0, -offset);
        let gridBottom = Math.min(this.contentHeight(), gridHeight - offset);
        if (gridBottom <= gridTop) {
            return;
        }

        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.separator);

        for (let bankIndex = 0; bankIndex <= this.bankCount(rows); bankIndex += 1) {
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
        for (let buttonIndex = 0; buttonIndex <= 3; buttonIndex += 1) {
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
                4 * BankManagerControlOptions.bankSize,
                1
            );
            mgraphics.fill();
        }
    }

    bankGridRight(rows)
    {
        return this.bankGridX(rows) +
            this.bankCount(rows) * BankManagerControlOptions.bankSize;
    }

    paintInstanceButtons(row, x, y)
    {
        [
            { label: "S", active: Boolean(row.solo), color: BankManagerControlOptions.solo },
            { label: "M", active: Boolean(row.mute), color: BankManagerControlOptions.mute },
            { label: "R", active: Boolean(this.actionFlash["instance:" + row.instanceId]), color: UiColors.devices.reset },
            { label: "B", active: Boolean(row.bypass), color: UiColors.devices.reset }
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
            let scope = this.presentation.scopeAction || {};
            let groupText = scope.active && scope.color
                ? scope.color
                : button.active
                    ? BankManagerControlOptions.background
                    : UiColors.base.activeText;
            mgraphics.set_source_rgba.apply(mgraphics, groupText);
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

    paintActions(x, height)
    {
        let group = this.presentation.groupAction || {};
        let ungroup = this.presentation.ungroupAction || {};
        let clear = this.presentation.clearAction || {};
        let scope = this.presentation.scopeAction || {};
        let actions = [
            { key: "group", label: "Group", action: group, momentary: true },
            { key: "ungroup", label: "Ungroup", action: ungroup, momentary: true },
            { key: "clear", label: "Clear", action: clear, momentary: true },
            {
                key: "scope",
                label: scope.active ? "Group" : "Local",
                action: scope,
                momentary: false
            }
        ];
        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.background);
        mgraphics.rectangle(
            x,
            0,
            BankManagerControlOptions.actionColumnWidth,
            height
        );
        mgraphics.fill();

        let paintButton = (entry, buttonY) => {
            let buttonBottom = Math.round(buttonY + BankManagerControlOptions.actionButtonHeight);
            let actualHeight = buttonBottom - buttonY;
            let borderColor = BankManagerControlOptions.separator;
            let textColor = entry.action.enabled
                ? UiColors.base.activeText
                : UiColors.base.disabledText;
            let flashed = entry.momentary && this.actionFlash[entry.key];
            let fillColor = flashed
                ? BankManagerControlOptions.focused
                : entry.action.active && entry.action.color
                    ? entry.action.color : BankManagerControlOptions.background;
            mgraphics.set_source_rgba.apply(mgraphics, fillColor);
            mgraphics.rectangle(x, buttonY, BankManagerControlOptions.actionColumnWidth,
                actualHeight);
            mgraphics.fill();
            mgraphics.set_source_rgba.apply(mgraphics, borderColor);
            mgraphics.rectangle(x, buttonY, BankManagerControlOptions.actionColumnWidth, 1);
            mgraphics.fill();
            mgraphics.rectangle(x, buttonBottom - 1,
                BankManagerControlOptions.actionColumnWidth, 1);
            mgraphics.fill();
            mgraphics.rectangle(x, buttonY, 1, actualHeight);
            mgraphics.fill();
            mgraphics.rectangle(x + BankManagerControlOptions.actionColumnWidth - 1,
                buttonY, 1, actualHeight);
            mgraphics.fill();
            mgraphics.set_source_rgba.apply(mgraphics,
                flashed || entry.action.active
                    ? BankManagerControlOptions.background : textColor);
            mgraphics.select_font_face("Arial");
            mgraphics.set_font_size(9);
            let textSize = mgraphics.text_measure(entry.label);
            mgraphics.move_to(x + (BankManagerControlOptions.actionColumnWidth - textSize[0]) / 2,
                buttonY + 12);
            mgraphics.show_text(entry.label);
        };

        actions.forEach((entry, index) => {
            paintButton(
                entry,
                Math.round(index * (BankManagerControlOptions.actionButtonHeight +
                    BankManagerControlOptions.actionGap))
            );
        });

        let history = this.presentation.history || {};
        let historyY = this.actionGroupHeight() + BankManagerControlOptions.historyGroupGap;
        [
            {
                key: "historyRedo",
                label: "Redo",
                action: { enabled: Boolean(history.canRedo) },
                momentary: true
            },
            {
                key: "historyUndo",
                label: "Undo",
                action: { enabled: Boolean(history.canUndo) },
                momentary: true
            }
        ].forEach((button, index) => {
            paintButton(
                button,
                historyY + index * (BankManagerControlOptions.actionButtonHeight +
                    BankManagerControlOptions.actionGap)
            );
        });
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

    markerAt(x)
    {
        let index = Math.floor(
            (x - this.markerGridX(this.presentation.rows || [])) /
            BankManagerControlOptions.bankSize
        );
        return index >= 0 && index < BankManagerControlOptions.processorMarkerIds.length
            ? BankManagerControlOptions.processorMarkerIds[index]
            : null;
    }

    selectAt(x, y, extendSelection, controlClick)
    {
        let clickX = x;
        let clickY = y;
        if (x < BankManagerControlOptions.outerPadding ||
                y < BankManagerControlOptions.outerPadding ||
                x >= mgraphics.size[0] - BankManagerControlOptions.outerPadding ||
                y >= mgraphics.size[1] - BankManagerControlOptions.outerPadding) {
            return;
        }
        x -= BankManagerControlOptions.outerPadding;
        y -= BankManagerControlOptions.outerPadding;
        this.debug("click x=" + clickX + " y=" + clickY +
            " normalizedX=" + x + " normalizedY=" + y +
            " controlClick=" + Boolean(controlClick));
        if (!this.presentation.enabled) {
            return;
        }

        if (x >= this.actionsColumnX() &&
                x < this.actionsColumnX() + BankManagerControlOptions.actionColumnWidth &&
                y >= 0 && y < this.contentHeight()) {
            let actionStep = BankManagerControlOptions.actionButtonHeight +
                BankManagerControlOptions.actionGap;
            let actionButtonIndex = Math.floor(y / actionStep);
            let actionGroupHeight = this.actionGroupHeight();
            if (y < actionGroupHeight &&
                    y >= actionButtonIndex * actionStep + BankManagerControlOptions.actionButtonHeight) {
                return;
            }
            let actions = [
                this.presentation.groupAction,
                this.presentation.ungroupAction,
                this.presentation.clearAction,
                this.presentation.scopeAction
            ];
            if (y < actionGroupHeight) {
                let action = actions[actionButtonIndex];
                if (action && action.enabled) {
                    if (actionButtonIndex < 3) {
                        this.flashAction(["group", "ungroup", "clear"][actionButtonIndex]);
                    }
                    this.emit([
                        "groupRequested",
                        "ungroupRequested",
                        "clearRequested",
                        "scopeToggled"
                    ][actionButtonIndex]);
                }
                return;
            }

            let historyY = actionGroupHeight + BankManagerControlOptions.historyGroupGap;
            let historyButtonIndex = Math.floor((y - historyY) / actionStep);
            if (y < historyY || historyButtonIndex < 0 || historyButtonIndex >= 2 ||
                    y >= historyY + historyButtonIndex * actionStep +
                        BankManagerControlOptions.actionButtonHeight) {
                return;
            }
            let history = this.presentation.history || {};
            let historyEnabled = [Boolean(history.canRedo), Boolean(history.canUndo)][historyButtonIndex];
            if (historyEnabled) {
                let cursor = Number(history.cursor) || 0;
                let target = historyButtonIndex === 0 ? cursor + 1 : cursor - 1;
                this.flashAction(["historyRedo", "historyUndo"][historyButtonIndex]);
                this.emit("historySelected", [target]);
            }
            return;
        }

        let rows = this.presentation.rows || [];
        let rowIndex = this.rowAt(y);
        let row = rows[rowIndex];
        if (!row) {
            return;
        }

        let processorId = this.markerAt(x);
        this.debug("rowIndex=" + rowIndex + " marker=" + processorId +
            " markerX=" + this.markerGridX(rows) +
            " bankX=" + this.bankGridX(rows));
        if (processorId) {
            let processor = (row.processors || []).filter((candidate) => {
                return candidate.processorId === processorId;
            })[0] || { processorId: processorId, bypassed: false };
            this.debug("processor click instanceId=" + row.instanceId +
                " processorId=" + processorId +
                " controlClick=" + Boolean(controlClick) +
                " currentBypassed=" + Boolean(processor.bypassed));
            if (controlClick) {
                if (this.doubleClick.isDoubleClick(
                        String(row.instanceId) + ":" + processorId)) {
                    this.emit("processorResetRequested", [processorId, row.instanceId]);
                    return;
                }
                    let bypassed = !this.bypassValue(
                        this.bypassKey(row.instanceId, processorId),
                        processor.bypassed
                    );
                    this.setBypassOverride(
                        this.bypassKey(row.instanceId, processorId),
                        bypassed
                    );
                    processor.bypassed = bypassed;
                this.debug("processor bypass emit instanceId=" + row.instanceId +
                    " processorId=" + processorId + " value=" + (bypassed ? 1 : 0));
                this.emit("processorBypassChanged", [
                    row.instanceId,
                    processorId,
                    bypassed ? 1 : 0
                ]);
                mgraphics.redraw();
                return;
            }
            this.emit("processorSelected", [row.instanceId, processorId]);
            if (this.doubleClick.isDoubleClick(
                    String(row.instanceId) + ":" + processorId)) {
                if (controlClick) {
                    this.emit("processorResetRequested", [processorId, row.instanceId]);
                }
            }
            return;
        }

        let instanceButtonsX = this.bankGridRight(rows) +
            BankManagerControlOptions.deviceColumnGap;
        if (x >= instanceButtonsX &&
                x < instanceButtonsX + BankManagerControlOptions.bankSize * 4) {
            let buttonIndex = Math.floor(
                (x - instanceButtonsX) / BankManagerControlOptions.bankSize
            );
            if (buttonIndex === 0) {
                this.emit("instanceSoloChanged", [
                    row.instanceId,
                    row.solo ? 0 : 1,
                    extendSelection ? 1 : 0
                ]);
            } else if (buttonIndex === 1) {
                this.emit("instanceMuteChanged", [
                    row.instanceId,
                    row.mute ? 0 : 1,
                    1
                ]);
            } else if (buttonIndex === 2) {
                this.flashAction("instance:" + row.instanceId);
                this.emit("instanceResetRequested");
            } else {
                this.emit("instanceBypassChanged", [
                    row.instanceId,
                    row.bypass ? 0 : 1,
                    1
                ]);
            }
            return;
        }

        let bankIndex = this.bankAt(row, x);
        if (bankIndex >= 0) {
            let bank = row.banks[bankIndex];
            if (bank.visible && bank.enabled) {
                this.debug("bank click instanceId=" + row.instanceId +
                    " bankId=" + bank.bankId +
                    " controlClick=" + Boolean(controlClick) +
                    " currentBypassed=" + Boolean(this.presentation.focusedBankBypassed));
                if (controlClick) {
                    if (this.doubleClick.isDoubleClick(
                            "bank:" + row.instanceId + ":" + bank.bankId)) {
                        this.emit("bankResetRequested", [row.instanceId, bank.bankId]);
                        return;
                    }
                    let bypassKey = this.bypassKey(row.instanceId, bank.bankId);
                    let bypassed = !this.bypassValue(
                        bypassKey,
                        bank.bypassed
                    );
                    this.setBypassOverride(bypassKey, bypassed);
                    this.debug("bank bypass emit instanceId=" + row.instanceId +
                        " bankId=" + bank.bankId + " value=" + (bypassed ? 1 : 0));
                    this.emit("bankBypassChanged", [
                        row.instanceId,
                        bank.bankId,
                        bypassed ? 1 : 0
                    ]);
                    mgraphics.redraw();
                    return;
                }
                this.emit("bankSelected", [
                    row.instanceId,
                    bank.bankId,
                    extendSelection ? 1 : 0
                ]);
                if (this.doubleClick.isDoubleClick(
                        "bank:" + row.instanceId + ":" + bank.bankId)) {
                    if (controlClick) {
                        this.emit("bankResetRequested", [row.instanceId, bank.bankId]);
                    }
                }
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

    beginPointer(x, y, shift)
    {
        if (!this.presentation.enabled) {
            return;
        }

        this.pointerDown = true;
        this.pointerClickHandled = false;
        this.pointerX = x;
        this.pointerY = y;
        this.pointerShift = Number(shift) !== 0;
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
            );
        }

        this.pointerDown = false;
        this.pointerShift = false;
        this.pointerClickHandled = false;
    }

    cancelPointer()
    {
        if (this.dragging) {
            this.endGesture();
        }

        this.pointerDown = false;
        this.pointerShift = false;
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

    setBankBypass(value)
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.pendingPresentation.focusedBankBypassed = Number(value) !== 0;
    }

    addRow(index,
    instanceId,
    label,
    local,
    solo,
    mute,
    bypass)
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
            bypass: Number(bypass) !== 0,
            processors: [],
            banks: []
        };
    }

    addProcessor(rowIndex,
    processorId,
    effectActive,
    markerActive,
    bypassed)
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

    setClearAction(enabled)
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.pendingPresentation.clearAction = {
            enabled: Number(enabled) !== 0
        };
    }

    setScopeAction(enabled, active, hasColor, red, green, blue, alpha)
    {
        if (!this.pendingPresentation) {
            return;
        }

        this.pendingPresentation.scopeAction = {
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0,
            color: this.colorFromArguments(hasColor, red, green, blue, alpha)
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

    patchBankBypass(value)
    {
        this.presentation.focusedBankBypassed = Number(value) !== 0;
    }

    patchRow(index,
    instanceId,
    label,
    local,
    solo,
    mute,
    bypass)
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
        row.bypass = Number(bypass) !== 0;
        if (!row.processors) row.processors = [];
    }

    patchProcessor(rowIndex,
    processorId,
    effectActive,
    markerActive,
    bypassed)
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
        this.confirmBypassOverride(
            this.bypassKey(row.instanceId, processor.processorId),
            processor.bypassed
        );
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
            bypassed: Number(args[11]) !== 0,
            color: this.colorFromArguments.apply(this, args.slice(12, 17)),
            textColor: this.colorFromArguments.apply(this, args.slice(17, 22))
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

    patchClearAction(enabled)
    {
        this.presentation.clearAction = {
            enabled: Number(enabled) !== 0
        };
    }

    patchScopeAction(enabled, active, hasColor, red, green, blue, alpha)
    {
        this.presentation.scopeAction = {
            enabled: Number(enabled) !== 0,
            active: Number(active) !== 0,
            color: this.colorFromArguments(hasColor, red, green, blue, alpha)
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

function bank_bypass(value) {
    bankManagerControl.setBankBypass(value);
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

function clear_action(enabled) {
    bankManagerControl.setClearAction(enabled);
}

function scope_action(enabled, active, hasColor, red, green, blue, alpha) {
    bankManagerControl.setScopeAction(
        enabled, active, hasColor, red, green, blue, alpha);
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

function bank_bypass_patch(value) {
    bankManagerControl.patchBankBypass(value);
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

function clear_action_patch(enabled) {
    bankManagerControl.patchClearAction(enabled);
}

function scope_action_patch(enabled, active, hasColor, red, green, blue, alpha) {
    bankManagerControl.patchScopeAction(
        enabled, active, hasColor, red, green, blue, alpha);
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
    bankManagerControl.debug("onclick modifier1=" + modifier1 +
        " modifier2=" + modifier2 + " shift=" + shift +
        " button=" + button);
    bankManagerControl.beginPointer(x, y, shift);
    bankManagerControl.selectAt(
        x,
        y,
        shift,
        Number(modifier1) !== 0 || Number(modifier2) !== 0
    );
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
