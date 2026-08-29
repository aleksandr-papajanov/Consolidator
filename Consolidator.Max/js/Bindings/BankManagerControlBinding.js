const { ControlBinding } = require("./ControlBinding.js");

class BankManagerControlBinding extends ControlBinding
{
    constructor(controller, presenter, sendMessage)
    {
        super(presenter, sendMessage);
        this.controller = controller;
        this.hasPresentation = false;
        this.connectPresentation();
    }
    
    colorArguments(color)
    {
        if (!color || color.length < 3) {
            return [0, 0, 0, 0, 0];
        }
        return [1, color[0], color[1], color[2],
            color[3] === undefined ? 1 : color[3]];
    }
    
    applyPresentation(presentation)
    {
        if (this.hasPresentation && presentation.delta) {
            this.applyDelta(presentation, presentation.delta);
            return;
        }
        this.send("presentation_begin", [
            presentation.enabled ? 1 : 0
        ]);
        (presentation.rows || []).forEach((row, rowIndex) => {
            this.send("row", [
                rowIndex,
                row.instanceId,
                row.label || "",
                row.local ? 1 : 0,
                row.solo ? 1 : 0,
                row.mute ? 1 : 0
            ]);
            this.sendProcessors("processor", row, rowIndex);
            (row.banks || []).forEach((bank) => {
                this.send("bank", [
                    rowIndex,
                    bank.bankId,
                    bank.label || "",
                    bank.system ? 1 : 0,
                    bank.visible ? 1 : 0,
                    bank.enabled ? 1 : 0,
                    bank.active ? 1 : 0,
                    bank.selected ? 1 : 0,
                    bank.opacity === undefined ? 1 : bank.opacity,
                    bank.groupId === undefined || bank.groupId === null
                        ? -1 : Number(bank.groupId),
                    bank.effectActive ? 1 : 0
                ].concat(
                    this.colorArguments(bank.color),
                    this.colorArguments(bank.textColor)
                ));
            });
        });
        let group = presentation.groupAction || {};
        let ungroup = presentation.ungroupAction || {};
        let clear = presentation.clearAction || {};
        this.send("group_action", [group.enabled ? 1 : 0, group.active ? 1 : 0]);
        this.send("ungroup_action", [ungroup.enabled ? 1 : 0, ungroup.active ? 1 : 0]);
        this.send("clear_action", [clear.enabled ? 1 : 0, clear.armed ? 1 : 0]);
        let history = presentation.history || {};
        this.send("history", [
            Number(history.cursor) || 0,
            Number(history.entryCount) || 0,
            history.canUndo ? 1 : 0,
            history.canRedo ? 1 : 0
        ]);
        this.send("selected_panel", [
            presentation.selectedPanel || "equalizer"
        ]);
        this.send("presentation_end");
        this.hasPresentation = true;
    }
    
    refreshPresentation()
    {
        let presentation = this.pendingPresentation ||
            (this.presenter && this.presenter.presentation);
        this.pendingPresentation = null;
        if (!presentation) {
            return;
        }
        this.hasPresentation = false;
        this.applyPresentation(presentation);
    }
    
    applyDelta(
        presentation,
        delta
    )
    {
        let rowIndex = Number(delta.rowIndex);
        this.send("presentation_patch_begin", [
            presentation.enabled ? 1 : 0
        ]);
    
        if (delta.selector === "bank_focus_changed") {
            this.sendFocusedBankPatch(
                presentation,
                delta.previousRowIndex,
                delta.previousBankId);
            this.sendFocusedBankPatch(
                presentation,
                delta.rowIndex,
                delta.bankId);
            this.send("presentation_patch_end");
            return;
        }
    
        if (delta.selector === "registry_instance_removed") {
            if (isFinite(rowIndex) && rowIndex >= 0) {
                this.send("row_remove", [rowIndex]);
            }
        } else {
            let row = (presentation.rows || [])[rowIndex];
            if (row) {
                this.sendRow("row_patch", row, rowIndex);
                if (delta.selector === "registry_instance_added") {
                    this.sendProcessors("processor_patch", row, rowIndex);
                    this.sendBanks("bank_patch", row, rowIndex);
                } else if (delta.selector === "registry_processor_changed") {
                    this.sendProcessors("processor_patch", row, rowIndex);
                } else if (delta.selector === "registry_bank_group_changed" ||
                        delta.selector === "registry_bank_effect_changed") {
                    let bankId = Number(delta.args[4]);
                    let bank = (row.banks || []).filter((candidate) => {
                        return Number(candidate.bankId) === bankId;
                    })[0];
                    if (bank) this.sendBank("bank_patch", bank, rowIndex);
                }
            }
        }
    
        if (delta.selector !== "registry_label_changed") {
            this.sendActions(presentation);
        }
        this.send("presentation_patch_end");
    }
    
    sendFocusedBankPatch(
        presentation,
        rowIndex,
        bankId
    )
    {
        let row = (presentation.rows || [])[Number(rowIndex)];
        if (!row) return;
        let targetBankId = Number(bankId);
        let bank = (row.banks || []).filter((candidate) => {
            return Number(candidate.bankId) === targetBankId;
        })[0];
        if (bank) this.sendBank("bank_patch", bank, Number(rowIndex));
    }
    
    sendRow(
        selector,
        row,
        rowIndex
    )
    {
        this.send(selector, [
            rowIndex,
            row.instanceId,
            row.label || "",
            row.local ? 1 : 0,
            row.solo ? 1 : 0,
            row.mute ? 1 : 0
        ]);
    }
    
    sendBank(
        selector,
        bank,
        rowIndex
    )
    {
        this.send(selector, [
            rowIndex,
            bank.bankId,
            bank.label || "",
            bank.system ? 1 : 0,
            bank.visible ? 1 : 0,
            bank.enabled ? 1 : 0,
            bank.active ? 1 : 0,
            bank.selected ? 1 : 0,
            bank.opacity === undefined ? 1 : bank.opacity,
            bank.groupId === undefined || bank.groupId === null
                ? -1 : Number(bank.groupId),
            bank.effectActive ? 1 : 0
        ].concat(
            this.colorArguments(bank.color),
            this.colorArguments(bank.textColor)
        ));
    }

    sendProcessor(
        selector,
        processor,
        rowIndex
    )
    {
        this.send(selector, [
            rowIndex,
            processor.processorId,
            processor.effectActive ? 1 : 0,
            processor.bypassed ? 1 : 0,
            processor.soloed ? 1 : 0
        ]);
    }

    sendProcessors(
        selector,
        row,
        rowIndex
    )
    {
        (row.processors || []).forEach((processor) => {
            this.sendProcessor(selector, processor, rowIndex);
        });
    }
    
    sendBanks(
        selector,
        row,
        rowIndex
    )
    {
        (row.banks || []).forEach((bank) => {
            this.sendBank(selector, bank, rowIndex);
        });
    }
    
    sendActions(presentation)
    {
        let group = presentation.groupAction || {};
        let ungroup = presentation.ungroupAction || {};
        let clear = presentation.clearAction || {};
        this.send("group_action_patch", [
            group.enabled ? 1 : 0,
            group.active ? 1 : 0
        ]);
        this.send("ungroup_action_patch", [
            ungroup.enabled ? 1 : 0,
            ungroup.active ? 1 : 0
        ]);
        this.send("clear_action_patch", [
            clear.enabled ? 1 : 0,
            clear.armed ? 1 : 0
        ]);
        let history = presentation.history || {};
        this.send("history_patch", [
            Number(history.cursor) || 0,
            Number(history.entryCount) || 0,
            history.canUndo ? 1 : 0,
            history.canRedo ? 1 : 0
        ]);
    }
    
    handleIntent(name, values)
    {
        this.controller.handleIntent(name, values);
    }
}

module.exports = {
    BankManagerControlBinding: BankManagerControlBinding
};
