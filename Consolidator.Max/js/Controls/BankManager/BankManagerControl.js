autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { BankManagerPresentation } = require("../../Presenters/BankManager/BankManagerPresentation.js");

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
    actionPanelHeight: 18,
    historyPanelHeight: 18,
    historySlotCount: 10,
    scrollbarWidth: 8,
    scrollbarGap: 2,
    scrollbarTrack: [0.14, 0.14, 0.14, 1],
    scrollbarThumb: [0.38, 0.38, 0.38, 1],
    scrollbarThumbActive: [0.55, 0.55, 0.55, 1],
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
        this.pointerShift = false;
        this.pointerGroup = false;
        this.dragging = false;
        this.lastY = 0;
        this.scrollbarDragging = false;
        this.scrollbarDragOffset = 0;
    }

    applyPresentation(presentation)
    {
        if (!presentation) {
            return;
        }

        this.presentation = presentation;
        if (!presentation.enabled) {
            this.dragging = false;
            this.scrollbarDragging = false;
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
        return Math.max(0, mgraphics.size[1] -
            BankManagerControlOptions.actionPanelHeight -
            BankManagerControlOptions.historyPanelHeight);
    }

    scrollbarGeometry()
    {
        let width = mgraphics.size[0];
        let height = this.contentHeight();
        let maximumOffset = this.maximumScrollOffset();
        if (maximumOffset <= 0 || height <= 0) {
            return null;
        }

        let contentHeight = (this.presentation.rows || []).length *
            BankManagerControlOptions.rowHeight;
        let thumbHeight = Math.max(
            24,
            height * height / contentHeight
        );
        thumbHeight = Math.min(height, thumbHeight);
        let trackHeight = height - thumbHeight;
        let thumbY = trackHeight <= 0
            ? 0
            : this.scrollOffset() / maximumOffset * trackHeight;

        return {
            x: width - BankManagerControlOptions.scrollbarGap -
                BankManagerControlOptions.scrollbarWidth,
            y: 0,
            width: BankManagerControlOptions.scrollbarWidth,
            height: height,
            thumbY: thumbY,
            thumbHeight: thumbHeight
        };
    }

    scrollbarAt(x, y)
    {
        let geometry = this.scrollbarGeometry();
        return geometry &&
            x >= geometry.x &&
            x < geometry.x + geometry.width &&
            y >= geometry.y &&
            y <= geometry.y + geometry.height;
    }

    updateScrollFromScrollbar(y)
    {
        let geometry = this.scrollbarGeometry();
        if (!geometry) {
            return;
        }

        let trackHeight = geometry.height - geometry.thumbHeight;
        let thumbY = Math.max(
            0,
            Math.min(
                trackHeight,
                y - this.scrollbarDragOffset - geometry.y
            )
        );
        this.scrollPosition = trackHeight <= 0
            ? 0
            : thumbY / trackHeight * this.maximumScrollOffset();
        mgraphics.redraw();
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
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
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
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(BankManagerControlOptions.fontSize);

        let contentHeight = this.contentHeight();
        this.paintActions(width, contentHeight);
        this.paintHistory(width, contentHeight +
            BankManagerControlOptions.actionPanelHeight);
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

            for (let bankIndex = 0; bankIndex < row.banks.length; bankIndex += 1) {
                this.paintBank(
                    row.banks[bankIndex],
                    100 + bankIndex * (
                        BankManagerControlOptions.bankSize +
                        BankManagerControlOptions.bankGap
                    ),
                    y,
                    highlightedGroups[String(row.banks[bankIndex].groupId)] === true
                );
            }
            this.paintInstanceButtons(row, this.bankGridRight(rows) + 4, y);
        }

        this.paintScrollbar();

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

        let gridX = 100;
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

        let buttonsX = this.bankGridRight(rows) + 4;
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
        return 100 + bankCount * BankManagerControlOptions.bankSize;
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

    paintScrollbar()
    {
        let geometry = this.scrollbarGeometry();
        if (!geometry) {
            return;
        }

        mgraphics.set_source_rgba.apply(
            mgraphics,
            BankManagerControlOptions.scrollbarTrack
        );
        mgraphics.rectangle(
            geometry.x,
            geometry.y,
            geometry.width,
            geometry.height
        );
        mgraphics.fill();

        mgraphics.set_source_rgba.apply(
            mgraphics,
            this.scrollbarDragging
                ? BankManagerControlOptions.scrollbarThumbActive
                : BankManagerControlOptions.scrollbarThumb
        );
        mgraphics.rectangle(
            geometry.x,
            geometry.thumbY,
            geometry.width,
            geometry.thumbHeight
        );
        mgraphics.fill();
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
        let buttonWidth = width / actions.length;
        let y = contentHeight;
        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.background);
        mgraphics.rectangle(0, y, width, BankManagerControlOptions.actionPanelHeight);
        mgraphics.fill();
        actions.forEach((entry, index) => {
            let x = index * buttonWidth;
            let borderColor = BankManagerControlOptions.separator;
            let textColor = entry.action.enabled
                ? BankManagerControlOptions.actionText
                : BankManagerControlOptions.separator;
            mgraphics.set_source_rgba.apply(mgraphics, borderColor);
            mgraphics.rectangle(x + 1, y + 1, buttonWidth - 2, 16);
            mgraphics.stroke();
            mgraphics.set_source_rgba.apply(mgraphics, textColor);
            mgraphics.select_font_face("Arial");
            mgraphics.set_font_size(9);
            let textSize = mgraphics.text_measure(entry.label);
            mgraphics.move_to(x + (buttonWidth - textSize[0]) / 2, y + 12);
            mgraphics.show_text(entry.label);
        });
    }

    paintHistory(width, y)
    {
        let history = this.presentation.history || {};
        let cursor = Number(history.cursor) || 0;
        let entryCount = Number(history.entryCount) || 0;
        let cellSize = BankManagerControlOptions.bankSize;
        let gridX = 100;
        let historyHeight = BankManagerControlOptions.historyPanelHeight;

        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.background);
        mgraphics.rectangle(0, y, width, historyHeight);
        mgraphics.fill();
        mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.remote);
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(9);
        mgraphics.move_to(2, y + 12);
        mgraphics.show_text("History");

        for (let slot = 0;
                slot < BankManagerControlOptions.historySlotCount;
                slot += 1) {
            let x = gridX + slot * cellSize;
            let available = slot <= entryCount;
            let current = slot === cursor;
            let applied = available && slot < cursor;
            let fillColor = current
                ? BankManagerControlOptions.focused
                : applied
                    ? BankManagerControlOptions.remote
                    : null;
            if (fillColor) {
                mgraphics.set_source_rgba.apply(mgraphics, fillColor);
                mgraphics.rectangle(x, y, cellSize, historyHeight);
                mgraphics.fill();
            }
        }

        mgraphics.set_source_rgba.apply(mgraphics,
            BankManagerControlOptions.separator);
        for (let slot = 0;
                slot <= BankManagerControlOptions.historySlotCount;
                slot += 1) {
            let x = gridX + slot * cellSize;
            mgraphics.rectangle(x, y, 1, historyHeight);
            mgraphics.fill();
        }
        mgraphics.rectangle(
            gridX,
            y,
            BankManagerControlOptions.historySlotCount * cellSize,
            1
        );
        mgraphics.fill();
        mgraphics.rectangle(
            gridX,
            y + historyHeight - 1,
            BankManagerControlOptions.historySlotCount * cellSize,
            1
        );
        mgraphics.fill();

        for (let slot = 0;
                slot < BankManagerControlOptions.historySlotCount;
                slot += 1) {
            let x = gridX + slot * cellSize;
            let available = slot <= entryCount;
            let current = slot === cursor;
            mgraphics.set_source_rgba.apply(mgraphics,
                current || available
                    ? BankManagerControlOptions.background
                    : BankManagerControlOptions.disabled);
            mgraphics.set_font_size(8);
            let label = String(slot);
            let textSize = mgraphics.text_measure(label);
            mgraphics.move_to(x + (cellSize - textSize[0]) / 2, y + 12);
            mgraphics.show_text(label);
        }
    }

    historyAt(x, y)
    {
        let historyY = this.contentHeight() +
            BankManagerControlOptions.actionPanelHeight;
        if (y < historyY || y >= historyY +
                BankManagerControlOptions.historyPanelHeight || x < 100) {
            return -1;
        }
        let slot = Math.floor((x - 100) / BankManagerControlOptions.bankSize);
        let entryCount = Number((this.presentation.history || {}).entryCount) || 0;
        return slot >= 0 && slot <= entryCount &&
            slot < BankManagerControlOptions.historySlotCount ? slot : -1;
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
            (x - 100) /
            (BankManagerControlOptions.bankSize + BankManagerControlOptions.bankGap)
        );

        return index >= 0 && row && row.banks[index] ? index : -1;
    }

    selectAt(x, y, extendSelection, groupControl)
    {
        if (!this.presentation.enabled) {
            return;
        }

        if (this.scrollbarAt(x, y)) {
            return;
        }

        let historySlot = this.historyAt(x, y);
        if (historySlot >= 0) {
            this.emit("historySelected", [historySlot]);
            return;
        }

        if (y >= this.contentHeight() &&
                y < this.contentHeight() + BankManagerControlOptions.actionPanelHeight) {
            let buttonIndex = Math.floor(x / (mgraphics.size[0] / 3));
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

        let instanceButtonsX = this.bankGridRight(rows) + 4;
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
        this.pointerX = x;
        this.pointerY = y;
        this.pointerShift = Number(shift) !== 0;
        this.pointerGroup = Number(groupControl) !== 0;
        this.dragging = false;
        this.scrollbarDragging = this.scrollbarAt(x, y);
        if (this.scrollbarDragging) {
            let geometry = this.scrollbarGeometry();
            this.scrollbarDragOffset = Math.max(
                0,
                Math.min(
                    geometry.thumbHeight,
                    y - geometry.thumbY
                )
            );
            this.updateScrollFromScrollbar(y);
        }
    }

    movePointer(x, y)
    {
        if (!this.pointerDown) {
            return;
        }

        if (this.scrollbarDragging) {
            this.updateScrollFromScrollbar(y);
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
        } else {
            if (!this.scrollbarDragging) {
                this.selectAt(
                    x,
                    y,
                    this.pointerShift,
                    this.pointerGroup
                );
            }
        }

        this.pointerDown = false;
        this.pointerShift = false;
        this.pointerGroup = false;
        this.scrollbarDragging = false;
    }

    cancelPointer()
    {
        if (this.dragging) {
            this.endGesture();
        }

        this.pointerDown = false;
        this.pointerShift = false;
        this.pointerGroup = false;
        this.scrollbarDragging = false;
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
            banks: []
        };
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

function presentation_end() {
    bankManagerControl.endPresentation();
}

function presentation_patch_begin(enabled) {
    bankManagerControl.beginPresentationPatch(enabled);
}

function row_patch(...args) {
    bankManagerControl.patchRow(...args);
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

function onclick(x, y, button, modifier1, shift) {
    bankManagerControl.beginPointer(x, y, shift, modifier1);
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
