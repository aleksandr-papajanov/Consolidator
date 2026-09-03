const { BankManagerContext } = require("./BankManagerContext.js");
class BankManagerController
{
    constructor(context)
    {
        this.context = context;
        this.historyJumpPending = false;
        this.unsubscribeHistory = context.transactions &&
            typeof context.transactions.subscribeHistory === "function"
            ? context.transactions.subscribeHistory((history) => {
                if (!this.historyJumpPending) return;
                this.historyJumpPending = false;
                this.refreshHistoryTarget();
            }, false) : null;
    }

    debug(message)
    {
        if (typeof post === "function") {
            post("[Consolidator][TrackName] BankManagerController " + message + "\n");
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

    selectProcessor(instanceId, processorId)
    {
        let current = this.context.viewModel.focusedSelection;
        this.debug("processorSelected instanceId=" + instanceId +
            " processorId=" + processorId +
            " current=" + JSON.stringify(current));
        if (!current) {
            this.debug("ignored: no focused selection");
            return;
        }
        if (String(current.instanceId) === String(instanceId)) {
            this.debug("same instance: selectPanel " + processorId);
            this.selectPanel(processorId);
            return;
        }

        let row = this.context.viewModel.rows.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        let bank = row && row.banks.filter((candidate) => {
            return Number(candidate.bankId) === Number(current.bankId);
        })[0];
        if (!bank) {
            this.debug("ignored: bank index " + current.bankId +
                " not found on instance " + instanceId);
            return;
        }

        this.debug("different instance: select bank=" + bank.bankId +
            " and panel=" + processorId);
        this.context.viewModel.setFocusedBank(instanceId, bank.bankId);
        this.context.uiTarget.show(
            instanceId,
            bank.bankId,
            processorId,
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
                Number(focusedBank.groupId) < 0) {
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
        this.context.transactions.begin((historyId) => {
            this.writeEntriesNow(entriesByInstance);
            this.context.transactions.end(historyId);
        });
    }

    writeEntriesNow(entriesByInstance)
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
    
    clearGroups()
    {
        this.context.transactions.begin((historyId) => {
            this.context.protocol.request("clear_topology", [], () => {
                this.context.transactions.end(historyId);
            });
        });
    }
    
    handleIntent(name, values)
    {
        values = values || [];
        switch (name) {
        case "bankSelected":
            this.selectBank(values[0], values[1], Number(values[2]) !== 0);
            break;
        case "panelSelected": this.selectPanel(values[0]); break;
        case "processorSelected":
            this.selectProcessor(values[0], values[1]);
            break;
        case "rowSelected": this.selectRow(values[0]); break;
        case "groupRequested": this.groupSelectedBanks(); break;
        case "ungroupRequested": this.ungroupFocusedBank(); break;
        case "clearRequested": this.clearGroups(); break;
        case "scopeToggled":
            this.context.viewModel.toggleScope();
            break;
        case "historySelected":
            this.jumpHistory(values[0]);
            break;
        case "instanceSoloChanged":
            this.setSolo(
                values[0],
                Number(values[1]) !== 0,
                Number(values[2]) !== 0
            );
            break;
        case "instanceMuteChanged":
            this.setMute(
                values[0],
                Number(values[1]) !== 0,
                Number(values[2]) !== 0
            );
            break;
        case "instanceBypassChanged":
            this.setBypass(
                values[0],
                Number(values[1]) !== 0,
                Number(values[2]) !== 0
            );
            break;
        case "instanceResetRequested":
            this.resetInstance();
            break;
        case "processorBypassChanged":
            this.debug("processorBypassChanged values=" + JSON.stringify(values));
            if (values.length >= 3) {
                this.setProcessorBypass(
                    values[0],
                    values[1],
                    Number(values[2]) !== 0
                );
            } else {
                let current = this.context.viewModel.focusedSelection;
                if (current) {
                    this.setProcessorBypass(
                        current.instanceId,
                        values[0],
                        Number(values[1]) !== 0
                    );
                }
            }
            break;
        case "bankBypassChanged":
            this.debug("bankBypassChanged values=" + JSON.stringify(values));
            if (values.length >= 3) {
                this.setBankBypass(
                    values[0],
                    values[1],
                    Number(values[2]) !== 0
                );
            } else {
                let current = this.context.viewModel.focusedSelection;
                if (current) {
                    this.setBankBypass(
                        current.instanceId,
                        current.bankId,
                        Number(values[0]) !== 0
                    );
                }
            }
            break;
        case "bankResetRequested":
            this.resetBank(values[0], values[1]);
            break;
        case "processorResetRequested":
            this.resetProcessor(values[0], values[1]);
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
        this.historyJumpPending = true;
        let requestId = this.context.transactions.jumpHistory(target, (response) => {
            if (response && response.error) {
                this.historyJumpPending = false;
                return;
            }
            if (this.historyJumpPending) {
                this.historyJumpPending = false;
                this.refreshHistoryTarget();
            }
        });
    }

    refreshHistoryTarget()
    {
        let targetState = this.context.uiTarget &&
            this.context.uiTarget.targetState;
        let selectedTarget = targetState && targetState.target;
        if (!selectedTarget) {
            return;
        }
        this.context.uiTarget.show(
            selectedTarget.instanceId,
            selectedTarget.bankId,
            selectedTarget.snapshotContext
        );
    }


    setMute(instanceId, value, additive)
    {
        let scope = this.resolveInstanceControlScope(instanceId, additive);
        this.context.protocol.request(
            "set_instance_mute",
            [instanceId,
                scope,
                value ? 1 : 0,
                additive ? "additive" : "exclusive"
            ]
        );
    }

    setBypass(instanceId, value, additive)
    {
        let scope = this.resolveInstanceControlScope(instanceId, additive);
        this.context.protocol.request(
            "set_instance_bypass",
            [instanceId,
                scope,
                value ? 1 : 0,
                additive ? "additive" : "exclusive"
            ]
        );
    }

    resetInstance()
    {
        this.context.state.reset(
            "dsp",
            undefined,
            0,
            this.context.scope.mode === "group" ? "group_instance" : "local"
        );
    }

    setProcessorBypass(instanceId, processorId, value)
    {
        this.context.protocol.request("set_processor_bypass",
            [instanceId, processorId, this.context.scope.mode, value ? 1 : 0]);
    }

    setBankBypass(instanceId, bankId, value)
    {
        this.context.protocol.request(
            "set_bank_bypass",
            [instanceId, bankId, this.context.scope.mode, value ? 1 : 0]);
    }


    resetProcessor(processorId, instanceId)
    {
        let paths = {
            input: "input_gain",
            saturator: "saturator",
            compressor: "compressor",
            equalizer: "equalizer",
            polish: "polish",
            output: "output_gain"
        };
        let path = paths[String(processorId)];
        if (!path) return;
        if (instanceId === undefined || instanceId === null) return;
        this.context.state.resetTargeted(
            instanceId, null, path, undefined, 0, this.context.scope.mode);
    }

    resetBank(instanceId, bankId)
    {
        if (instanceId === undefined || instanceId === null ||
                bankId === undefined || bankId === null) return;
        this.context.state.resetTargeted(
            instanceId, Number(bankId), "equalizer.bank", undefined,
            0, this.context.scope.mode);
    }

    setSolo(instanceId, value, additive)
    {
        let scope = this.resolveInstanceControlScope(instanceId, additive);
        this.context.protocol.request(
            "set_instance_solo",
            [instanceId,
                scope,
                value ? 1 : 0,
                additive ? "additive" : "exclusive"
            ]
        );
    }

    destroy()
    {
        if (this.unsubscribeHistory) this.unsubscribeHistory();
        this.unsubscribeHistory = null;
        this.historyJumpPending = false;
        this.context = null;
    }

    resolveInstanceControlScope(instanceId, additive)
    {
        let focusedBank = typeof this.context.viewModel.focusedBankFor === "function"
            ? this.context.viewModel.focusedBankFor(instanceId) : null;
        let hasFocusedGroup = focusedBank && focusedBank.groupId !== undefined &&
            focusedBank.groupId !== null && Number(focusedBank.groupId) >= 0;
        return additive && !hasFocusedGroup
            ? "local" : this.context.scope.mode;
    }
}

module.exports = {
    BankManagerController: BankManagerController
};
