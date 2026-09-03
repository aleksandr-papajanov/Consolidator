const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");
const { BankManagerLayout } = require("./BankManagerLayout.js");
const { BankManagerHitActions } = require("./BankManagerHitActions.js");
const { BankManagerPointerGesture } = require("./BankManagerPointerGesture.js");

class BankManagerInteraction
{
    constructor(getPresentation, getViewportSize, emit, redraw, feedback)
    {
        this.getPresentation = getPresentation;
        this.getViewportSize = getViewportSize;
        this.emit = emit;
        this.redraw = redraw;
        this.feedback = feedback;
        this.scrollPosition = 0;
        this.actions = new BankManagerHitActions(emit, redraw, feedback);
        this.pointer = new BankManagerPointerGesture(
            () => Boolean(this.getPresentation().enabled),
            (x, y, shift, control) => this.selectAt(x, y, shift, control),
            (delta) => this.scrollBy(delta),
            emit
        );
    }

    get dragging() { return this.pointer.dragging; }
    set dragging(value) { this.pointer.dragging = Boolean(value); }

    layout()
    {
        let size = this.getViewportSize();
        let layout = new BankManagerLayout(size[0], size[1],
            this.getPresentation().rows || [], this.scrollPosition);
        this.scrollPosition = layout.scrollPosition;
        return layout;
    }

    scrollBy(delta)
    {
        let layout = this.layout();
        let next = Math.max(0, Math.min(layout.maximumScrollOffset(),
            layout.scrollPosition + delta));
        if (next !== this.scrollPosition) {
            this.scrollPosition = next;
            this.redraw();
        }
    }

    selectAt(pointerX, pointerY, extendSelection, controlClick)
    {
        let options = BankManagerControlOptions;
        let size = this.getViewportSize();
        if (pointerX < options.outerPadding || pointerY < options.outerPadding ||
                pointerX >= size[0] - options.outerPadding ||
                pointerY >= size[1] - options.outerPadding) return;
        let x = pointerX - options.outerPadding;
        let y = pointerY - options.outerPadding;
        let presentation = this.getPresentation();
        if (!presentation.enabled) return;
        let layout = this.layout();
        if (this.actions.selectAction(presentation, layout, x, y)) return;
        let row = (presentation.rows || [])[layout.rowAt(y)];
        if (!row) return;
        let processorId = layout.markerAt(x);
        if (processorId) {
            this.actions.selectProcessor(row, processorId, controlClick);
            return;
        }
        if (this.actions.selectInstance(row, layout, x, extendSelection)) return;
        let bankIndex = layout.bankAt(row, x);
        if (bankIndex >= 0) {
            this.actions.selectBank(
                row, row.banks[bankIndex], extendSelection, controlClick);
        }
        else {
            this.emit("rowSelected", [row.instanceId]);
        }
    }

    beginPointer(x, y, shift) { this.pointer.begin(x, y, shift); }
    markClickHandled() { this.pointer.markClickHandled(); }
    movePointer(x, y) { this.pointer.move(x, y); }
    endPointer(x, y) { this.pointer.end(x, y); }
    cancelPointer() { this.pointer.cancel(); }
    resetPointer() { this.pointer.reset(); }

    drag(y)
    {
        if (!this.pointer.dragging) return;
        this.scrollBy(this.pointer.lastY - y);
        this.pointer.lastY = y;
    }
}

module.exports = {
    BankManagerInteraction: BankManagerInteraction
};
