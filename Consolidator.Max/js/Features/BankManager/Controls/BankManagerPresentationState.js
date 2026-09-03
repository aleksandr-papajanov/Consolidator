const { BankManagerPresentation } = require(
    "../Presenters/BankManagerPresentation.js");
const {
    colorFromArguments,
    createAction,
    createBank,
    createHistory,
    createProcessor,
    createRow
} = require("./BankManagerPresentationFactory.js");
const { BankManagerPresentationPatcher } = require(
    "./BankManagerPresentationPatcher.js");

class BankManagerPresentationState
{
    constructor(onApplied, onBypassConfirmed)
    {
        this.presentation = new BankManagerPresentation();
        this.pendingPresentation = null;
        this.onApplied = onApplied;
        this.onBypassConfirmed = onBypassConfirmed;
        this.patcher = this.createPatcher();
    }

    createPatcher()
    {
        return new BankManagerPresentationPatcher(
            this.presentation, this.onBypassConfirmed);
    }

    apply(presentation)
    {
        if (!presentation) return;
        this.confirmBypassValues(presentation);
        this.presentation = presentation;
        this.patcher = this.createPatcher();
        this.onApplied(presentation);
    }

    confirmBypassValues(presentation)
    {
        (presentation.rows || []).forEach((row) => {
            (row.processors || []).forEach((processor) => {
                this.onBypassConfirmed(
                    row.instanceId, processor.processorId, processor.bypassed);
            });
            (row.banks || []).forEach((bank) => {
                this.onBypassConfirmed(row.instanceId, bank.bankId, bank.bypassed);
            });
        });
    }

    begin(enabled)
    {
        this.pendingPresentation = new BankManagerPresentation();
        this.pendingPresentation.enabled = Number(enabled) !== 0;
    }

    setBankBypass(value)
    {
        if (this.pendingPresentation) {
            this.pendingPresentation.focusedBankBypassed = Number(value) !== 0;
        }
    }

    addRow(index, ...args)
    {
        if (this.pendingPresentation) {
            this.pendingPresentation.rows[Number(index)] = createRow.apply(null, args);
        }
    }

    addProcessor(rowIndex, ...args)
    {
        let row = this.pendingPresentation &&
            this.pendingPresentation.rows[Number(rowIndex)];
        if (row) row.processors.push(createProcessor.apply(null, args));
    }

    addBank(rowIndex, ...args)
    {
        let row = this.pendingPresentation &&
            this.pendingPresentation.rows[Number(rowIndex)];
        if (row) row.banks.push(createBank(args));
    }

    setAction(name, enabled, active, color)
    {
        if (this.pendingPresentation) {
            this.pendingPresentation[name] = createAction(enabled, active, color);
        }
    }

    setHistory(cursor, entryCount, canUndo, canRedo)
    {
        if (this.pendingPresentation) {
            this.pendingPresentation.history = createHistory(
                cursor, entryCount, canUndo, canRedo);
        }
    }

    setSelectedPanel(panel)
    {
        if (this.pendingPresentation) {
            this.pendingPresentation.selectedPanel = String(panel);
        }
    }

    end()
    {
        let presentation = this.pendingPresentation;
        this.pendingPresentation = null;
        this.apply(presentation);
    }

    beginPatch(enabled)
    {
        this.presentation.enabled = Number(enabled) !== 0;
    }
    patchBankBypass(value) {
        this.presentation.focusedBankBypassed = Number(value) !== 0;
    }
    patchRow(index, ...args) { this.patcher.row(index, args); }
    patchProcessor(rowIndex, ...args) { this.patcher.processor(rowIndex, args); }
    removeRow(index) { this.patcher.removeRow(index); }
    patchBank(...args) { this.patcher.bank(args); }
    patchAction(name, enabled, active, color) {
        this.patcher.action(name, enabled, active, color);
    }
    patchHistory(cursor, entryCount, canUndo, canRedo) {
        this.patcher.history(cursor, entryCount, canUndo, canRedo);
    }
    createHistory(cursor, entryCount, canUndo, canRedo) {
        return createHistory(cursor, entryCount, canUndo, canRedo);
    }
}

module.exports = {
    BankManagerPresentationState: BankManagerPresentationState,
    colorFromArguments: colorFromArguments
};
