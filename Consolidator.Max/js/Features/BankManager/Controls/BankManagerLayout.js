const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");

class BankManagerLayout
{
    constructor(width, height, rows, scrollPosition)
    {
        this.width = Math.max(0, Number(width) - BankManagerControlOptions.outerPadding * 2);
        this.height = Math.max(0, Number(height) - BankManagerControlOptions.outerPadding * 2);
        this.rows = rows || [];
        this.scrollPosition = Math.max(0, Math.min(
            this.maximumScrollOffset(),
            Number(scrollPosition) || 0
        ));
    }

    bankCount()
    {
        return this.rows.reduce((count, row) => {
            return Math.max(count, (row.banks || []).length);
        }, 0);
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

    actionsColumnX()
    {
        return Math.max(0, this.width - BankManagerControlOptions.actionColumnWidth);
    }

    markerGridX()
    {
        const bankWidth = this.bankCount() * BankManagerControlOptions.bankSize;
        const chainWidth = this.markerWidth() + BankManagerControlOptions.columnGap +
            bankWidth + BankManagerControlOptions.deviceColumnGap +
            this.instanceButtonWidth();

        return Math.max(0, this.actionsColumnX() - BankManagerControlOptions.columnGap -
            chainWidth);
    }

    bankGridX()
    {
        return this.markerGridX() + this.markerWidth() +
            BankManagerControlOptions.columnGap;
    }

    bankGridRight()
    {
        return this.bankGridX() +
            this.bankCount() * BankManagerControlOptions.bankSize;
    }

    instanceButtonsX()
    {
        return this.bankGridRight() + BankManagerControlOptions.deviceColumnGap;
    }

    labelWidth()
    {
        return Math.max(0, this.markerGridX() - BankManagerControlOptions.columnGap);
    }

    actionGroupHeight()
    {
        const actionCount = 4;
        return actionCount * BankManagerControlOptions.actionButtonHeight +
            (actionCount - 1) * BankManagerControlOptions.actionGap;
    }

    maximumScrollOffset()
    {
        const contentHeight = this.rows.length * BankManagerControlOptions.rowHeight;
        return Math.max(0, contentHeight - this.height);
    }

    rowAt(y)
    {
        return Math.floor(
            (y + this.scrollPosition) / BankManagerControlOptions.rowHeight
        );
    }

    bankAt(row, x)
    {
        const index = Math.floor(
            (x - this.bankGridX()) /
            (BankManagerControlOptions.bankSize + BankManagerControlOptions.bankGap)
        );

        return index >= 0 && row && row.banks[index] ? index : -1;
    }

    markerAt(x)
    {
        const index = Math.floor(
            (x - this.markerGridX()) / BankManagerControlOptions.bankSize
        );

        return index >= 0 && index < BankManagerControlOptions.processorMarkerIds.length
            ? BankManagerControlOptions.processorMarkerIds[index]
            : null;
    }
}

module.exports = {
    BankManagerLayout: BankManagerLayout
};
