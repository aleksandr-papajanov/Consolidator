const {
    createAction,
    createBank,
    createHistory,
    createProcessor,
    createRow
} = require("./BankManagerPresentationFactory.js");

class BankManagerPresentationPatcher
{
    constructor(presentation, confirmBypass)
    {
        this.presentation = presentation;
        this.confirmBypass = confirmBypass;
    }

    row(index, args)
    {
        let rowIndex = Number(index);
        let existing = this.presentation.rows[rowIndex];
        let next = createRow.apply(null, args);
        next.banks = existing && existing.banks || [];
        next.processors = existing && existing.processors || [];
        this.presentation.rows[rowIndex] = next;
    }

    processor(rowIndex, args)
    {
        let row = this.presentation.rows[Number(rowIndex)];
        if (!row) return;
        let next = createProcessor.apply(null, args);
        let index = (row.processors || []).findIndex((candidate) => {
            return candidate.processorId === next.processorId;
        });
        if (index >= 0) row.processors[index] = next;
        else {
            row.processors = row.processors || [];
            row.processors.push(next);
        }
        this.confirmBypass(row.instanceId, next.processorId, next.bypassed);
    }

    bank(args)
    {
        let row = this.presentation.rows[Number(args[0])];
        if (!row) return;
        let bank = createBank(args, 1);
        let bankId = Number(bank.bankId);
        let index = row.banks.findIndex((candidate) => {
            return Number(candidate.bankId) === bankId;
        });
        if (index >= 0) row.banks[index] = bank;
        else row.banks.push(bank);
        this.confirmBypass(row.instanceId, bank.bankId, bank.bypassed);
    }

    removeRow(index)
    {
        let rowIndex = Number(index);
        if (rowIndex >= 0 && rowIndex < this.presentation.rows.length) {
            this.presentation.rows.splice(rowIndex, 1);
        }
    }

    action(name, enabled, active, color)
    {
        this.presentation[name] = createAction(enabled, active, color);
    }

    history(cursor, entryCount, canUndo, canRedo)
    {
        this.presentation.history = createHistory(
            cursor, entryCount, canUndo, canRedo);
    }
}

module.exports = {
    BankManagerPresentationPatcher: BankManagerPresentationPatcher
};
