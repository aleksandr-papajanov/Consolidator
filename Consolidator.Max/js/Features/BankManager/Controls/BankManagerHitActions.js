const { DoubleClickTracker } = require(
    "../../../Shared/Controls/DoubleClickTracker.js");
const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");

class BankManagerHitActions
{
    constructor(emit, redraw, feedback)
    {
        this.emit = emit;
        this.redraw = redraw;
        this.feedback = feedback;
        this.doubleClick = new DoubleClickTracker();
    }

    selectAction(presentation, layout, x, y)
    {
        let options = BankManagerControlOptions;
        if (x < layout.actionsColumnX() ||
                x >= layout.actionsColumnX() + options.actionColumnWidth ||
                y < 0 || y >= layout.height) return false;
        let step = options.actionButtonHeight + options.actionGap;
        let actionGroupHeight = layout.actionGroupHeight();
        if (y < actionGroupHeight) {
            let index = Math.floor(y / step);
            if (y >= index * step + options.actionButtonHeight) return true;
            let action = [
                presentation.groupAction,
                presentation.ungroupAction,
                presentation.clearAction,
                presentation.scopeAction
            ][index];
            if (action && action.enabled) {
                let keys = ["group", "ungroup", "clear"];
                if (index < keys.length) this.feedback.flashAction(keys[index]);
                this.emit(["groupRequested", "ungroupRequested",
                    "clearRequested", "scopeToggled"][index]);
            }
            return true;
        }

        let historyY = actionGroupHeight + options.historyGroupGap;
        let index = Math.floor((y - historyY) / step);
        if (y < historyY || index < 0 || index >= 2 ||
                y >= historyY + index * step + options.actionButtonHeight) {
            return true;
        }
        let history = presentation.history || {};
        if ([Boolean(history.canRedo), Boolean(history.canUndo)][index]) {
            let cursor = Number(history.cursor) || 0;
            this.feedback.flashAction(["historyRedo", "historyUndo"][index]);
            this.emit("historySelected", [index === 0 ? cursor + 1 : cursor - 1]);
        }
        return true;
    }

    selectProcessor(row, processorId, controlClick)
    {
        let processor = (row.processors || []).find((candidate) => {
            return candidate.processorId === processorId;
        }) || { processorId: processorId, bypassed: false };
        let clickKey = String(row.instanceId) + ":" + processorId;
        if (controlClick) {
            if (this.doubleClick.isDoubleClick(clickKey)) {
                this.emit("processorResetRequested", [processorId, row.instanceId]);
                return;
            }
            let bypassed = !this.feedback.bypassValue(
                row.instanceId, processorId, processor.bypassed);
            this.feedback.setBypassOverride(row.instanceId, processorId, bypassed);
            processor.bypassed = bypassed;
            this.emit("processorBypassChanged",
                [row.instanceId, processorId, bypassed ? 1 : 0]);
            this.redraw();
            return;
        }
        this.emit("processorSelected", [row.instanceId, processorId]);
        this.doubleClick.isDoubleClick(clickKey);
    }

    selectInstance(row, layout, x, extendSelection)
    {
        if (x < layout.instanceButtonsX() ||
                x >= layout.instanceButtonsX() + layout.instanceButtonWidth()) {
            return false;
        }
        let index = Math.floor((x - layout.instanceButtonsX()) /
            BankManagerControlOptions.bankSize);
        if (index === 0) {
            this.emit("instanceSoloChanged",
                [row.instanceId, row.solo ? 0 : 1, extendSelection ? 1 : 0]);
        }
        else if (index === 1) {
            this.emit("instanceMuteChanged", [row.instanceId, row.mute ? 0 : 1, 1]);
        }
        else if (index === 2) {
            this.feedback.flashAction("instance:" + row.instanceId);
            this.emit("instanceResetRequested");
        }
        else {
            this.emit("instanceBypassChanged",
                [row.instanceId, row.bypass ? 0 : 1, 1]);
        }
        return true;
    }

    selectBank(row, bank, extendSelection, controlClick)
    {
        if (!bank.visible || !bank.enabled) return;
        let clickKey = "bank:" + row.instanceId + ":" + bank.bankId;
        if (controlClick) {
            if (this.doubleClick.isDoubleClick(clickKey)) {
                this.emit("bankResetRequested", [row.instanceId, bank.bankId]);
                return;
            }
            let bypassed = !this.feedback.bypassValue(
                row.instanceId, bank.bankId, bank.bypassed);
            this.feedback.setBypassOverride(row.instanceId, bank.bankId, bypassed);
            this.emit("bankBypassChanged",
                [row.instanceId, bank.bankId, bypassed ? 1 : 0]);
            this.redraw();
            return;
        }
        this.emit("bankSelected",
            [row.instanceId, bank.bankId, extendSelection ? 1 : 0]);
        this.doubleClick.isDoubleClick(clickKey);
    }
}

module.exports = {
    BankManagerHitActions: BankManagerHitActions
};
