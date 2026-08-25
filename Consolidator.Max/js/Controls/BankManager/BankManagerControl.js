autowatch = 1;
inlets = 1;
outlets = 1;

include("Project:/js/Presenters/BankManager/BankManagerPresentation.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var BankManagerControlOptions = {
    background: [0.08, 0.08, 0.08, 1],
    text: [0.8, 0.8, 0.8, 1],
    focused: [0.35, 0.7, 1, 1],
    remote: [0.55, 0.55, 0.55, 1],
    action: [0.35, 0.7, 1, 1],
    actionActive: [0.95, 0.65, 0.25, 1],
    disabled: [0.25, 0.25, 0.25, 1],
    separator: [0.25, 0.25, 0.25, 1],
    padding: 0,
    rowHeight: 17,
    bankSize: 16,
    bankGap: 2,
    actionHeight: 16,
    actionGap: 2,
    linkGroupStartY: 40,
    linkGroupCellHeight: 16,
    fontSize: 11
};

function BankManagerControl() {
    this.presentation = new BankManagerPresentation();
    this.pendingPresentation = null;
    this.scrollPosition = 0;
    this.pointerDown = false;
    this.pointerX = 0;
    this.pointerY = 0;
    this.pointerShift = false;
    this.dragging = false;
    this.lastY = 0;
}

BankManagerControl.prototype.applyPresentation = function (presentation) {
    if (!presentation) {
        return;
    }

    this.presentation = presentation;
    if (!presentation.enabled) {
        this.dragging = false;
    }

    mgraphics.redraw();
};

BankManagerControl.prototype.scrollOffset = function () {
    this.scrollPosition = Math.max(
        0,
        Math.min(this.maximumScrollOffset(), this.scrollPosition)
    );

    return this.scrollPosition;
};

BankManagerControl.prototype.maximumScrollOffset = function () {
    var rows = this.presentation.rows || [];
    var contentHeight = rows.length * BankManagerControlOptions.rowHeight;
    return Math.max(0, contentHeight - mgraphics.size[1]);
};

BankManagerControl.prototype.emit = function (name, payload) {
    if (payload === undefined) {
        outlet(0, name);
    } else if (payload instanceof Array) {
        outlet(0, [name].concat(payload));
    } else {
        outlet(0, [name, payload]);
    }
};

BankManagerControl.prototype.paintBank = function (bank, x, y) {
    if (!bank.visible) {
        return;
    }

    var fallbackColor = bank.system
        ? BankManagerControlOptions.text
        : BankManagerControlOptions.focused;
    var color = bank.color || fallbackColor;
    var alpha = bank.opacity === undefined ? 1 : bank.opacity;
    var displayColor = [
        color[0],
        color[1],
        color[2],
        (color[3] === undefined ? 1 : color[3]) * alpha
    ];
    var fallbackTextColor = bank.active
        ? BankManagerControlOptions.background
        : displayColor;
    var textColor = bank.textColor || fallbackTextColor;

    mgraphics.set_source_rgba.apply(mgraphics, displayColor);
    mgraphics.rectangle(
        x,
        y,
        BankManagerControlOptions.bankSize,
        BankManagerControlOptions.bankSize
    );
    if (bank.active) {
        mgraphics.fill();
    } else {
        mgraphics.stroke();
    }

    mgraphics.set_source_rgba.apply(mgraphics, textColor);
    mgraphics.select_font_face("Arial");
    mgraphics.set_font_size(9);
    mgraphics.move_to(x + 4, y + 11);
    mgraphics.show_text(String(bank.label));
};

BankManagerControl.prototype.paint = function () {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var rows = this.presentation.rows || [];
    var offset = this.scrollOffset();

    mgraphics.set_source_rgba.apply(
        mgraphics,
        BankManagerControlOptions.background
    );
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    mgraphics.select_font_face("Arial");
    mgraphics.set_font_size(BankManagerControlOptions.fontSize);

    this.paintActions(width);

    for (var rowIndex = 0; rowIndex < rows.length; rowIndex += 1) {
        var row = rows[rowIndex];
        var y = rowIndex * BankManagerControlOptions.rowHeight - offset;
        if (y + BankManagerControlOptions.rowHeight < 0 || y > height) {
            continue;
        }

        if (row.local) {
            mgraphics.set_source_rgba.apply(mgraphics, [0.12, 0.12, 0.12, 1]);
            mgraphics.rectangle(
                0,
                y,
                width - 44,
                BankManagerControlOptions.rowHeight
            );
            mgraphics.fill();
        }

        mgraphics.set_source_rgba.apply(
            mgraphics,
            row.local
                ? BankManagerControlOptions.focused
                : BankManagerControlOptions.remote
        );
        mgraphics.move_to(2, y + 12);
        mgraphics.show_text(row.label);

        for (var bankIndex = 0; bankIndex < row.banks.length; bankIndex += 1) {
            this.paintBank(
                row.banks[bankIndex],
                100 + bankIndex * (
                    BankManagerControlOptions.bankSize +
                    BankManagerControlOptions.bankGap
                ),
                y
            );
        }

        mgraphics.set_source_rgba.apply(
            mgraphics,
            BankManagerControlOptions.separator
        );
        mgraphics.rectangle(
            0,
            y + BankManagerControlOptions.rowHeight - 1,
            width,
            1
        );
        mgraphics.fill();
    }

    var groups = this.presentation.linkGroups || [];
    for (var groupIndex = 0; groupIndex < groups.length; groupIndex += 1) {
        var group = groups[groupIndex];
        var groupX = width - 44 + (groupIndex % 2) * 22;
        var groupY = BankManagerControlOptions.linkGroupStartY +
            Math.floor(groupIndex / 2) *
            BankManagerControlOptions.linkGroupCellHeight;
        if (!group.color) {
            continue;
        }

        mgraphics.set_source_rgba.apply(mgraphics, group.color);
        mgraphics.rectangle(groupX, groupY, 16, 14);
        if (group.active) {
            mgraphics.fill();
        } else {
            mgraphics.stroke();
        }

        var groupTextColor = BankManagerControlOptions.disabled;
        if (group.active) {
            groupTextColor = BankManagerControlOptions.background;
        } else if (group.enabled) {
            groupTextColor = group.color;
        }

        mgraphics.set_source_rgba.apply(mgraphics, groupTextColor);
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(9);
        mgraphics.move_to(groupX + 5, groupY + 10);
        mgraphics.show_text(String(group.label));
    }
};

BankManagerControl.prototype.paintActions = function (width) {
    var edit = this.presentation.editAction || {};
    var clear = this.presentation.clearAction || {};
    var x = width - 44;

    var editColor = BankManagerControlOptions.disabled;
    if (edit.enabled) {
        editColor = edit.active
            ? BankManagerControlOptions.actionActive
            : BankManagerControlOptions.action;
    }

    mgraphics.set_source_rgba.apply(mgraphics, editColor);
    mgraphics.rectangle(x, 0, 44, BankManagerControlOptions.actionHeight);
    if (edit.active && edit.enabled) {
        mgraphics.fill();
    } else {
        mgraphics.stroke();
    }

    mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.text);
    mgraphics.move_to(x + 4, 11);
    mgraphics.show_text("edit");

    var clearColor = clear.enabled
        ? BankManagerControlOptions.action
        : BankManagerControlOptions.disabled;
    mgraphics.set_source_rgba.apply(mgraphics, clearColor);
    mgraphics.rectangle(
        x,
        BankManagerControlOptions.actionHeight +
            BankManagerControlOptions.actionGap,
        44,
        BankManagerControlOptions.actionHeight
    );
    if (clear.armed && clear.enabled) {
        mgraphics.fill();
    } else {
        mgraphics.stroke();
    }

    mgraphics.set_source_rgba.apply(mgraphics, BankManagerControlOptions.text);
    mgraphics.move_to(
        x + 4,
        BankManagerControlOptions.actionHeight +
            BankManagerControlOptions.actionGap +
            11
    );
    mgraphics.show_text(clear.armed ? "sure?" : "clear");
};

BankManagerControl.prototype.rowAt = function (y) {
    return Math.floor(
        (y + this.scrollOffset()) / BankManagerControlOptions.rowHeight
    );
};

BankManagerControl.prototype.bankAt = function (row, x) {
    var index = Math.floor(
        (x - 100) /
        (BankManagerControlOptions.bankSize + BankManagerControlOptions.bankGap)
    );

    return index >= 0 && row && row.banks[index] ? index : -1;
};

BankManagerControl.prototype.selectAt = function (x, y, extendSelection) {
    if (!this.presentation.enabled) {
        return;
    }

    if (x >= mgraphics.size[0] - 44 && y < 16) {
        if (
            this.presentation.editAction &&
            this.presentation.editAction.enabled
        ) {
            this.emit("editToggled");
        }

        return;
    }

    if (
        x >= mgraphics.size[0] - 44 &&
        y >= BankManagerControlOptions.actionHeight +
            BankManagerControlOptions.actionGap &&
        y < BankManagerControlOptions.actionHeight * 2 +
            BankManagerControlOptions.actionGap
    ) {
        if (
            this.presentation.clearAction &&
            this.presentation.clearAction.enabled
        ) {
            this.emit("clearRequested");
        }

        return;
    }

    if (
        x >= mgraphics.size[0] - 44 &&
        y >= BankManagerControlOptions.linkGroupStartY
    ) {
        var groupIndex = Math.floor(
            (y - BankManagerControlOptions.linkGroupStartY) /
                BankManagerControlOptions.linkGroupCellHeight
        ) * 2 + (x >= mgraphics.size[0] - 22 ? 1 : 0);
        var group = (this.presentation.linkGroups || [])[groupIndex];
        if (group && group.enabled) {
            this.emit("linkGroupSelected", group.linkId);
        }

        return;
    }

    var rows = this.presentation.rows || [];
    var rowIndex = this.rowAt(y);
    var row = rows[rowIndex];
    if (!row) {
        return;
    }

    var bankIndex = this.bankAt(row, x);
    if (bankIndex >= 0) {
        var bank = row.banks[bankIndex];
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
};

BankManagerControl.prototype.beginGesture = function (y) {
    if (!this.presentation.enabled) {
        return;
    }

    this.dragging = true;
    this.lastY = y;
    this.emit("gestureBegan");
};

BankManagerControl.prototype.beginPointer = function (x, y, shift) {
    if (!this.presentation.enabled) {
        return;
    }

    this.pointerDown = true;
    this.pointerX = x;
    this.pointerY = y;
    this.pointerShift = Number(shift) !== 0;
    this.dragging = false;
};

BankManagerControl.prototype.movePointer = function (x, y) {
    if (!this.pointerDown) {
        return;
    }

    if (!this.dragging) {
        var distance = Math.sqrt(
            Math.pow(x - this.pointerX, 2) + Math.pow(y - this.pointerY, 2)
        );
        if (distance < 4) {
            return;
        }

        this.beginGesture(this.pointerY);
    }

    this.drag(y);
};

BankManagerControl.prototype.endPointer = function (x, y) {
    if (!this.pointerDown) {
        return;
    }

    if (this.dragging) {
        this.endGesture();
    } else {
        this.selectAt(x, y, this.pointerShift);
    }

    this.pointerDown = false;
    this.pointerShift = false;
};

BankManagerControl.prototype.cancelPointer = function () {
    if (this.dragging) {
        this.endGesture();
    }

    this.pointerDown = false;
    this.pointerShift = false;
};

BankManagerControl.prototype.drag = function (y) {
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
};

BankManagerControl.prototype.endGesture = function () {
    if (!this.dragging) {
        return;
    }

    this.dragging = false;
    this.emit("gestureEnded");
};

BankManagerControl.prototype.colorFromArguments = function (
    hasColor,
    red,
    green,
    blue,
    alpha
) {
    if (Number(hasColor) === 0) {
        return null;
    }

    return [
        Number(red),
        Number(green),
        Number(blue),
        Number(alpha)
    ];
};

BankManagerControl.prototype.beginPresentation = function (
    enabled,
    linkEditing
) {
    this.pendingPresentation = new BankManagerPresentation();
    this.pendingPresentation.enabled = Number(enabled) !== 0;
    this.pendingPresentation.linkEditing = Number(linkEditing) !== 0;
};

BankManagerControl.prototype.addRow = function (
    index,
    instanceId,
    label,
    local
) {
    if (!this.pendingPresentation) {
        return;
    }

    var presentation = this.pendingPresentation;
    presentation.rows[Number(index)] = {
        instanceId: instanceId,
        label: String(label),
        local: Number(local) !== 0,
        banks: []
    };
};

BankManagerControl.prototype.addBank = function (
    rowIndex,
    bankId,
    label,
    system,
    visible,
    enabled,
    active,
    opacity,
    hasColor,
    red,
    green,
    blue,
    alpha,
    hasTextColor,
    textRed,
    textGreen,
    textBlue,
    textAlpha
) {
    if (!this.pendingPresentation) {
        return;
    }

    var targetRow = this.pendingPresentation.rows[Number(rowIndex)];
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
        opacity: Number(opacity),
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
};

BankManagerControl.prototype.addLinkGroup = function (
    linkId,
    label,
    active,
    used,
    enabled,
    hasColor,
    red,
    green,
    blue,
    alpha
) {
    if (!this.pendingPresentation) {
        return;
    }

    this.pendingPresentation.linkGroups.push({
        linkId: linkId,
        label: String(label),
        active: Number(active) !== 0,
        used: Number(used) !== 0,
        enabled: Number(enabled) !== 0,
        color: this.colorFromArguments(
            hasColor,
            red,
            green,
            blue,
            alpha
        )
    });
};

BankManagerControl.prototype.setEditAction = function (enabled, active) {
    if (!this.pendingPresentation) {
        return;
    }

    this.pendingPresentation.editAction = {
        enabled: Number(enabled) !== 0,
        active: Number(active) !== 0
    };
};

BankManagerControl.prototype.setClearAction = function (enabled, armed) {
    if (!this.pendingPresentation) {
        return;
    }

    this.pendingPresentation.clearAction = {
        enabled: Number(enabled) !== 0,
        armed: Number(armed) !== 0
    };
};

BankManagerControl.prototype.endPresentation = function () {
    if (!this.pendingPresentation) {
        return;
    }

    this.applyPresentation(this.pendingPresentation);
    this.pendingPresentation = null;
};

BankManagerControl.prototype.beginPresentationPatch = function (
    enabled,
    linkEditing
) {
    this.presentation.enabled = Number(enabled) !== 0;
    this.presentation.linkEditing = Number(linkEditing) !== 0;
};

BankManagerControl.prototype.patchRow = function (
    index,
    instanceId,
    label,
    local
) {
    var rowIndex = Number(index);
    var row = this.presentation.rows[rowIndex];
    if (!row) {
        row = { banks: [] };
        this.presentation.rows[rowIndex] = row;
    }
    row.instanceId = instanceId;
    row.label = String(label);
    row.local = Number(local) !== 0;
};

BankManagerControl.prototype.removeRow = function (index) {
    var rowIndex = Number(index);
    if (rowIndex >= 0 && rowIndex < this.presentation.rows.length) {
        this.presentation.rows.splice(rowIndex, 1);
    }
};

BankManagerControl.prototype.patchBank = function () {
    var args = arrayfromargs(arguments);
    var rowIndex = Number(args[0]);
    var bankId = Number(args[1]);
    var row = this.presentation.rows[rowIndex];
    if (!row) return;

    var bank = {
        bankId: args[1],
        label: String(args[2]),
        system: Number(args[3]) !== 0,
        visible: Number(args[4]) !== 0,
        enabled: Number(args[5]) !== 0,
        active: Number(args[6]) !== 0,
        opacity: Number(args[7]),
        color: this.colorFromArguments.apply(this, args.slice(8, 13)),
        textColor: this.colorFromArguments.apply(this, args.slice(13, 18))
    };
    for (var index = 0; index < row.banks.length; index += 1) {
        if (Number(row.banks[index].bankId) === bankId) {
            row.banks[index] = bank;
            return;
        }
    }
    row.banks.push(bank);
};

BankManagerControl.prototype.patchLinkGroup = function () {
    var args = arrayfromargs(arguments);
    var linkId = Number(args[0]);
    var group = {
        linkId: args[0],
        label: String(args[1]),
        active: Number(args[2]) !== 0,
        used: Number(args[3]) !== 0,
        enabled: Number(args[4]) !== 0,
        color: this.colorFromArguments.apply(this, args.slice(5, 10))
    };
    for (var index = 0;
            index < this.presentation.linkGroups.length;
            index += 1) {
        if (Number(this.presentation.linkGroups[index].linkId) === linkId) {
            this.presentation.linkGroups[index] = group;
            return;
        }
    }
    this.presentation.linkGroups.push(group);
};

BankManagerControl.prototype.patchEditAction = function (enabled, active) {
    this.presentation.editAction = {
        enabled: Number(enabled) !== 0,
        active: Number(active) !== 0
    };
};

BankManagerControl.prototype.patchClearAction = function (enabled, armed) {
    this.presentation.clearAction = {
        enabled: Number(enabled) !== 0,
        armed: Number(armed) !== 0
    };
};

BankManagerControl.prototype.endPresentationPatch = function () {
    this.scrollOffset();
    mgraphics.redraw();
};

function presentation_begin(enabled, linkEditing) {
    bankManagerControl.beginPresentation(enabled, linkEditing);
}

function row(index, instanceId, label, local) {
    bankManagerControl.addRow(index, instanceId, label, local);
}

function bank() {
    bankManagerControl.addBank.apply(bankManagerControl, arguments);
}

function link_group() {
    bankManagerControl.addLinkGroup.apply(bankManagerControl, arguments);
}

function edit_action(enabled, active) {
    bankManagerControl.setEditAction(enabled, active);
}

function clear_action(enabled, armed) {
    bankManagerControl.setClearAction(enabled, armed);
}

function presentation_end() {
    bankManagerControl.endPresentation();
}

function presentation_patch_begin(enabled, linkEditing) {
    bankManagerControl.beginPresentationPatch(enabled, linkEditing);
}

function row_patch(index, instanceId, label, local) {
    bankManagerControl.patchRow(index, instanceId, label, local);
}

function row_remove(index) {
    bankManagerControl.removeRow(index);
}

function bank_patch() {
    bankManagerControl.patchBank.apply(bankManagerControl, arguments);
}

function link_group_patch() {
    bankManagerControl.patchLinkGroup.apply(bankManagerControl, arguments);
}

function edit_action_patch(enabled, active) {
    bankManagerControl.patchEditAction(enabled, active);
}

function clear_action_patch(enabled, armed) {
    bankManagerControl.patchClearAction(enabled, armed);
}

function presentation_patch_end() {
    bankManagerControl.endPresentationPatch();
}

function paint() {
    bankManagerControl.paint();
}

function onclick(x, y, button, modifier1, shift) {
    bankManagerControl.beginPointer(x, y, shift);
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

var bankManagerControl = new BankManagerControl();
