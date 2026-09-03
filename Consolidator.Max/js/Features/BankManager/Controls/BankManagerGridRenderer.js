const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");
const { fillRectangle } = require("./BankManagerDrawing.js");

class BankManagerGridRenderer
{
    paint(graphics, rows, layout)
    {
        if (layout.bankCount() > 0) {
            this.paintGrid(graphics, layout.bankGridX(),
                layout.bankCount() * BankManagerControlOptions.bankSize,
                layout.bankCount(), rows, layout);
            this.paintGrid(graphics, layout.instanceButtonsX(),
                layout.instanceButtonWidth(), 3, rows, layout);
        }
        if (rows.length > 0) {
            this.paintGrid(graphics, layout.markerGridX(), layout.markerWidth(),
                BankManagerControlOptions.processorMarkerIds.length,
                rows, layout);
        }
    }

    paintGrid(graphics, x, width, columnCount, rows, layout)
    {
        let gridHeight = rows.length * BankManagerControlOptions.rowHeight;
        let top = Math.max(0, -layout.scrollPosition);
        let bottom = Math.min(layout.height, gridHeight - layout.scrollPosition);
        if (bottom <= top) return;

        for (let column = 0; column <= columnCount; column += 1) {
            fillRectangle(graphics, BankManagerControlOptions.separator,
                x + column * BankManagerControlOptions.bankSize,
                top, 1, bottom - top);
        }
        for (let rowIndex = 0; rowIndex <= rows.length; rowIndex += 1) {
            let y = rowIndex * BankManagerControlOptions.rowHeight -
                layout.scrollPosition;
            if (y >= 0 && y <= layout.height) {
                fillRectangle(graphics, BankManagerControlOptions.separator,
                    x, y, width, 1);
            }
        }
    }
}

module.exports = {
    BankManagerGridRenderer: BankManagerGridRenderer
};
