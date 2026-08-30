const { BankManagerContext } = require("./BankManagerContext.js");

class BankManagerController
{
    constructor(context)
    {
        this.context = context;
        this.clearConfirmationArmed = false;
        this.clearConfirmationTimer = null;
    }
    
    updateClearAction(armed)
    {
        this.clearConfirmationArmed = armed;
        this.context.viewModel.apply({
            clearAction: {
                enabled: Boolean(this.context.viewModel.clearAction.enabled),
                armed: armed
            }
        });
    }
    
    disarmClearConfirmation()
    {
        this.clearConfirmationTimer = null;
        this.updateClearAction(false);
    }
    
    armClearConfirmation()
    {
        this.updateClearAction(true);
        if (this.clearConfirmationTimer !== null &&
                typeof clearTimeout === "function") {
            clearTimeout(this.clearConfirmationTimer);
        }
        if (typeof setTimeout === "function") {
            this.clearConfirmationTimer = setTimeout(() => {
                this.disarmClearConfirmation();
            }, 3000);
        }
    }
    
    selectBank(
        instanceId,
        bankId,
        extendSelection
    )
    {
        if (extendSelection) {
            if (!this.context.viewModel.canSelectBank(instanceId, bankId, true)) {
                return;
            }
            this.toggleBankSelection(instanceId, bankId, true);
            return;
        }
        let focusedChanged = this.context.viewModel.setFocusedBank(
            instanceId,
            bankId
        );
        if (this.context.viewModel.canSelectBank(instanceId, bankId, false)) {
            this.toggleBankSelection(instanceId, bankId, false);
        }
        if (focusedChanged !== false) {
            this.context.uiTarget.show(
                instanceId,
                bankId,
                this.context.viewModel.selectedPanel,
                (response) => this.acceptSnapshot(response));
        }
    }

    selectPanel(panel)
    {
        let target = this.context.uiTarget.targetState.target;
        if (!target) return;
        this.context.uiTarget.show(
            target.instanceId,
            target.bankId,
            panel,
            (response) => this.acceptSnapshot(response));
    }

    acceptSnapshot(response)
    {
        if (response && !response.error && response.snapshotContext) {
            this.context.viewModel.setSelectedPanel(response.snapshotContext);
            this.context.onSnapshotAccepted(response.snapshotContext);
        }
    }
    
    selectRow(instanceId)
    {
        let row = this.context.viewModel.rows.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        if (!row || !row.banks.length) return;
        this.selectBank(instanceId, row.banks[0].bankId);
    }
    
    toggleBankSelection(
        instanceId,
        bankId,
        extendSelection
    )
    {
        this.context.viewModel.toggleBankSelection(
            instanceId,
            bankId,
            extendSelection
        );
    }
    
    groupSelectedBanks()
    {
        if (this.context.viewModel.getSelectedBanks().length < 2) {
            return;
        }

        let groupId = this.context.viewModel.nextGroupId();
        if (groupId < 0) {
            return;
        }

        this.writeGroupForSelectedBanks(groupId);
    }

    ungroupFocusedBank()
    {
        let context = this.context;
        let focusedBank = context.viewModel.focusedBank();
        if (!focusedBank || focusedBank.groupId === null ||
                focusedBank.groupId === undefined ||
                Number(focusedBank.groupId) <= 0) {
            return;
        }

        let groupId = Number(focusedBank.groupId);
        let entriesByInstance = {};
        context.viewModel.rows.forEach((row) => {
            row.banks.forEach((bank) => {
                if (Number(bank.groupId) !== groupId) {
                    return;
                }

                let instanceId = String(row.instanceId);
                if (!entriesByInstance[instanceId]) {
                    entriesByInstance[instanceId] = [];
                }
                entriesByInstance[instanceId].push({
                    path: "bank." + bank.bankId + ".group",
                    value: null
                });
            });
        });
        this.writeEntries(entriesByInstance);
        context.viewModel.clearBankSelection();
    }

    writeGroupForSelectedBanks(groupId)
    {
        let context = this.context;
        let entriesByInstance = {};
        context.viewModel.getSelectedBanks()
            .forEach((selection) => {
                let instanceId = String(selection.instanceId);
                if (!entriesByInstance[instanceId]) {
                    entriesByInstance[instanceId] = [];
                }
                entriesByInstance[instanceId].push({
                    path: "bank." + selection.bankId + ".group",
                    value: Number(groupId)
                });
            });
        this.writeEntries(entriesByInstance);
        context.viewModel.clearBankSelection();
    }

    writeEntries(entriesByInstance)
    {
        let context = this.context;
        Object.keys(entriesByInstance).forEach((instanceId) => {
            let entries = entriesByInstance[instanceId];
            for (let offset = 0; offset < entries.length; offset += 16) {
                context.state.setManyTopologyFor(
                    instanceId,
                    entries.slice(offset, offset + 16)
                );
            }
        });
    }
    
    clearLocalGroups()
    {
        if (!this.clearConfirmationArmed) {
            this.armClearConfirmation();
            return;
        }
        this.disarmClearConfirmation();
        let row = this.context.viewModel.rows.filter((candidate) => {
            return candidate.local;
        })[0];
        let entries = (row ? row.banks : []).filter((bank) => {
            return bank.groupId !== undefined && bank.groupId !== null &&
                Number(bank.groupId) > 0;
        }).map((bank) => {
            return { path: "bank." + bank.bankId + ".group", value: null };
        });
        if (entries.length === 0) return;
        this.context.state.setManyTopologyFor(this.context.instanceId, entries);
    }
    
    handleIntent(name, values)
    {
        values = values || [];
        switch (name) {
        case "bankSelected":
            this.selectBank(values[0], values[1], Number(values[2]) !== 0);
            break;
        case "panelSelected": this.selectPanel(values[0]); break;
        case "rowSelected": this.selectRow(values[0]); break;
        case "groupRequested": this.groupSelectedBanks(); break;
        case "ungroupRequested": this.ungroupFocusedBank(); break;
        case "clearRequested": this.clearLocalGroups(); break;
        case "historySelected":
            this.jumpHistory(values[0]);
            break;
        case "instanceSoloChanged":
            this.setSolo(
                Number(values[0]) !== 0,
                Number(values[1]) !== 0,
                Number(values[2]) !== 0
            );
            break;
        case "instanceMuteChanged":
            this.setMute(
                Number(values[0]) !== 0,
                Number(values[1]) !== 0
            );
            break;
        case "instanceResetRequested":
            this.resetInstance(Number(values[0]) !== 0);
            break;
        case "processorBypassChanged":
            this.setProcessorBypass(values[0], Number(values[1]) !== 0,
                Number(values[2]) !== 0);
            break;
        case "processorSoloChanged":
            this.setProcessorSolo(values[0], Number(values[1]) !== 0,
                Number(values[2]) !== 0, Number(values[3]) !== 0);
            break;
        case "processorResetRequested":
            this.resetProcessor(values[0], Number(values[1]) !== 0);
            break;
        }
    }

    jumpHistory(cursor)
    {
        if (!this.context.transactions) {
            return;
        }
        let target = Number(cursor);
        if (!isFinite(target) || target < 0 ||
                target > this.context.transactions.history.entryCount) {
            return;
        }
        this.context.transactions.jumpHistory(target);
    }

    setMute(value, groupControl)
    {
        this.context.protocol.request(
            "set_instance_mute",
            [groupControl ? "group" : "local", value ? 1 : 0]
        );
    }

    resetInstance(groupControl)
    {
        this.context.state.reset(
            "dsp",
            undefined,
            0,
            groupControl ? "group_instance" : "local"
        );
    }

    setProcessorBypass(processorId, value, groupControl)
    {
        this.context.protocol.request("set_processor_bypass",
            [processorId, groupControl ? "group" : "local", value ? 1 : 0]);
    }

    setProcessorSolo(processorId, value, additive, groupControl)
    {
        this.context.protocol.request("set_processor_solo",
            [processorId, groupControl ? "group" : "local", value ? 1 : 0,
                additive ? "additive" : "exclusive"]);
    }

    resetProcessor(processorId, groupControl)
    {
        let paths = {
            input: "input_gain",
            saturator: "saturator",
            compressor: "compressor",
            equalizer: "equalizer",
            output: "output_gain"
        };
        let path = paths[String(processorId)];
        if (!path) return;
        this.context.state.reset(
            path,
            undefined,
            0,
            groupControl ? "group" : "local"
        );
    }

    setSolo(value, additive, groupControl)
    {
        this.context.protocol.request(
            "set_instance_solo",
            [groupControl ? "group" : "local",
                value ? 1 : 0,
                additive ? "additive" : "exclusive"
            ]
        );
    }

    
    destroy()
    {
        if (this.clearConfirmationTimer !== null &&
                typeof clearTimeout === "function") {
            clearTimeout(this.clearConfirmationTimer);
        }
        this.clearConfirmationTimer = null;
        this.context = null;
    }
}

module.exports = {
    BankManagerController: BankManagerController
};
