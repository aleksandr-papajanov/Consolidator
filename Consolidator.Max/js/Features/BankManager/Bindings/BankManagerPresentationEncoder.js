class BankManagerPresentationEncoder
{
    constructor(send)
    {
        this.send = send;
    }

    sendPresentation(presentation)
    {
        this.send("presentation_begin", [presentation.enabled ? 1 : 0]);
        this.send("bank_bypass", [presentation.focusedBankBypassed ? 1 : 0]);
        (presentation.rows || []).forEach((row, rowIndex) => {
            this.sendRow("row", row, rowIndex);
            this.sendProcessors("processor", row, rowIndex);
            this.sendBanks("bank", row, rowIndex);
        });
        this.sendFullActions(presentation);
        this.send("selected_panel", [presentation.selectedPanel || "equalizer"]);
        this.send("presentation_end");
    }

    sendDelta(presentation, delta)
    {
        this.send("presentation_patch_begin", [presentation.enabled ? 1 : 0]);
        if (delta.selector === "registry_processor_markers_changed")
        {
            (delta.rowIndices || []).forEach((rowIndex) => {
                const row = (presentation.rows || [])[Number(rowIndex)];
                if (row)
                {
                    this.sendProcessors("processor_patch", row, Number(rowIndex));
                }
            });
            this.send("presentation_patch_end");
            return;
        }
        if (delta.selector === "bank_bypass_changed")
        {
            this.send("bank_bypass_patch", [presentation.focusedBankBypassed ? 1 : 0]);
            this.send("presentation_patch_end");
            return;
        }
        if (delta.selector === "bank_focus_changed")
        {
            this.send("bank_bypass_patch", [0]);
            this.sendFocusedBank(presentation, delta.previousRowIndex, delta.previousBankId);
            this.sendFocusedBank(presentation, delta.rowIndex, delta.bankId);
            this.sendActionPatches(presentation);
            this.send("presentation_patch_end");
            return;
        }

        this.sendRegistryDelta(presentation, delta);
        if (delta.selector !== "registry_label_changed")
        {
            this.sendActionPatches(presentation);
        }
        this.send("presentation_patch_end");
    }

    sendRegistryDelta(presentation, delta)
    {
        const rowIndex = Number(delta.rowIndex);
        if (delta.selector === "registry_instance_removed")
        {
            if (isFinite(rowIndex) && rowIndex >= 0)
            {
                this.send("row_remove", [rowIndex]);
            }
            return;
        }

        const row = (presentation.rows || [])[rowIndex];
        if (!row)
        {
            return;
        }
        this.sendRow("row_patch", row, rowIndex);
        if (delta.selector === "registry_instance_added")
        {
            this.sendProcessors("processor_patch", row, rowIndex);
            this.sendBanks("bank_patch", row, rowIndex);
        }
        else if (delta.selector === "registry_processor_changed")
        {
            this.sendProcessors("processor_patch", row, rowIndex);
        }
        else if (delta.selector === "registry_bank_group_changed" ||
                delta.selector === "registry_bank_effect_changed" ||
                delta.selector === "registry_bank_bypass_changed")
        {
            const bankId = Number(delta.args[4]);
            const bank = (row.banks || []).find((candidate) => {
                return Number(candidate.bankId) === bankId;
            });
            if (bank)
            {
                this.sendBank("bank_patch", bank, rowIndex);
            }
        }
    }

    sendFocusedBank(presentation, rowIndex, bankId)
    {
        const row = (presentation.rows || [])[Number(rowIndex)];
        if (!row)
        {
            return;
        }
        const bank = (row.banks || []).find((candidate) => {
            return Number(candidate.bankId) === Number(bankId);
        });
        if (bank)
        {
            this.sendBank("bank_patch", bank, Number(rowIndex));
        }
    }

    sendRow(selector, row, rowIndex)
    {
        this.send(selector, [
            rowIndex,
            row.instanceId,
            row.label || "",
            row.local ? 1 : 0,
            row.solo ? 1 : 0,
            row.mute ? 1 : 0,
            row.bypass ? 1 : 0
        ]);
    }

    sendBank(selector, bank, rowIndex)
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
                ? -1
                : Number(bank.groupId),
            bank.effectActive ? 1 : 0,
            bank.bypassed ? 1 : 0
        ].concat(
            this.colorArguments(bank.color),
            this.colorArguments(bank.textColor)
        ));
    }

    sendProcessor(selector, processor, rowIndex)
    {
        this.send(selector, [
            rowIndex,
            processor.processorId,
            processor.effectActive ? 1 : 0,
            processor.markerActive ? 1 : 0,
            processor.bypassed ? 1 : 0
        ]);
    }

    sendProcessors(selector, row, rowIndex)
    {
        (row.processors || []).forEach((processor) => {
            this.sendProcessor(selector, processor, rowIndex);
        });
    }

    sendBanks(selector, row, rowIndex)
    {
        (row.banks || []).forEach((bank) => {
            this.sendBank(selector, bank, rowIndex);
        });
    }

    sendFullActions(presentation)
    {
        const group = presentation.groupAction || {};
        const ungroup = presentation.ungroupAction || {};
        const clear = presentation.clearAction || {};
        const scope = presentation.scopeAction || {};
        this.send("group_action", [group.enabled ? 1 : 0, group.active ? 1 : 0]);
        this.send("ungroup_action", [ungroup.enabled ? 1 : 0, ungroup.active ? 1 : 0]);
        this.send("clear_action", [clear.enabled ? 1 : 0]);
        this.send("scope_action", [scope.enabled ? 1 : 0, scope.active ? 1 : 0]
            .concat(this.colorArguments(scope.color)));
        this.sendHistory("history", presentation.history);
    }

    sendActionPatches(presentation)
    {
        const group = presentation.groupAction || {};
        const ungroup = presentation.ungroupAction || {};
        const clear = presentation.clearAction || {};
        const scope = presentation.scopeAction || {};
        this.send("group_action_patch", [group.enabled ? 1 : 0, group.active ? 1 : 0]);
        this.send("ungroup_action_patch", [
            ungroup.enabled ? 1 : 0,
            ungroup.active ? 1 : 0
        ]);
        this.send("clear_action_patch", [clear.enabled ? 1 : 0]);
        this.send("scope_action_patch", [scope.enabled ? 1 : 0, scope.active ? 1 : 0]
            .concat(this.colorArguments(scope.color)));
        this.sendHistory("history_patch", presentation.history);
    }

    sendHistory(selector, source)
    {
        const history = source || {};
        this.send(selector, [
            Number(history.cursor) || 0,
            Number(history.entryCount) || 0,
            history.canUndo ? 1 : 0,
            history.canRedo ? 1 : 0
        ]);
    }

    colorArguments(color)
    {
        if (!color || color.length < 3)
        {
            return [0, 0, 0, 0, 0];
        }
        return [
            1,
            color[0],
            color[1],
            color[2],
            color[3] === undefined ? 1 : color[3]
        ];
    }
}

module.exports = {
    BankManagerPresentationEncoder: BankManagerPresentationEncoder
};
