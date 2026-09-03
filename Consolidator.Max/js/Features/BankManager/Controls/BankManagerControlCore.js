const { BankManagerFeedback } = require("./BankManagerFeedback.js");
const { BankManagerInteraction } = require("./BankManagerInteraction.js");
const { BankManagerLayout } = require("./BankManagerLayout.js");
const {
    BankManagerPresentationState,
    colorFromArguments
} = require("./BankManagerPresentationState.js");
const { BankManagerRenderer } = require("./BankManagerRenderer.js");

class BankManagerControlCore
{
    constructor(graphics, emitOutlet, taskConstructor)
    {
        this.graphics = graphics;
        this.emitOutlet = emitOutlet;
        const redraw = () => {
            this.graphics.redraw();
        };
        this.feedback = new BankManagerFeedback(
            redraw,
            (callback) => new taskConstructor(callback, this)
        );
        this.presentationState = new BankManagerPresentationState(
            (presentation) => {
                if (!presentation.enabled)
                {
                    this.interaction.dragging = false;
                }
                redraw();
            },
            (instanceId, itemId, value) => {
                this.feedback.confirmBypassOverride(instanceId, itemId, value);
            }
        );
        this.interaction = new BankManagerInteraction(
            () => this.presentation,
            () => this.graphics.size,
            (name, payload) => this.emit(name, payload),
            redraw,
            this.feedback
        );
        this.renderer = new BankManagerRenderer();
    }

    get presentation()
    {
        return this.presentationState.presentation;
    }

    get pendingPresentation()
    {
        return this.presentationState.pendingPresentation;
    }

    applyPresentation(presentation)
    {
        this.presentationState.apply(presentation);
    }

    paint()
    {
        this.renderer.paint(
            this.graphics,
            this.presentation,
            this.interaction.layout(),
            this.feedback
        );
    }

    emit(name, payload)
    {
        this.emitOutlet(name, payload);
    }

    layout(rows)
    {
        return new BankManagerLayout(
            this.graphics.size[0],
            this.graphics.size[1],
            rows || this.presentation.rows || [],
            this.interaction.scrollPosition
        );
    }

    bankGridX(rows)
    {
        return this.layout(rows).bankGridX();
    }

    bankGridRight(rows)
    {
        return this.layout(rows).bankGridRight();
    }

    markerGridX(rows)
    {
        return this.layout(rows).markerGridX();
    }

    actionsColumnX()
    {
        return this.layout().actionsColumnX();
    }

    scrollBy(delta)
    {
        this.interaction.scrollBy(delta);
    }

    selectAt(x, y, extendSelection, controlClick)
    {
        this.interaction.selectAt(x, y, extendSelection, controlClick);
    }

    beginPointer(x, y, shift)
    {
        this.interaction.beginPointer(x, y, shift);
    }

    movePointer(x, y)
    {
        this.interaction.movePointer(x, y);
    }

    endPointer(x, y)
    {
        this.interaction.endPointer(x, y);
    }

    cancelPointer()
    {
        this.interaction.cancelPointer();
    }

    beginPresentation(enabled)
    {
        this.presentationState.begin(enabled);
    }

    setBankBypass(value)
    {
        this.presentationState.setBankBypass(value);
    }

    addRow(...args)
    {
        this.presentationState.addRow(...args);
    }

    addProcessor(...args)
    {
        this.presentationState.addProcessor(...args);
    }

    addBank(...args)
    {
        this.presentationState.addBank(...args);
    }

    setGroupAction(enabled, active)
    {
        this.presentationState.setAction("groupAction", enabled, active, null);
    }

    setUngroupAction(enabled, active)
    {
        this.presentationState.setAction("ungroupAction", enabled, active, null);
    }

    setClearAction(enabled)
    {
        this.presentationState.setAction("clearAction", enabled, false, null);
    }

    setScopeAction(enabled, active, hasColor, red, green, blue, alpha)
    {
        this.presentationState.setAction(
            "scopeAction",
            enabled,
            active,
            colorFromArguments(hasColor, red, green, blue, alpha)
        );
    }

    setHistory(...args)
    {
        this.presentationState.setHistory(...args);
    }

    setSelectedPanel(panel)
    {
        this.presentationState.setSelectedPanel(panel);
    }

    endPresentation()
    {
        this.presentationState.end();
    }

    beginPresentationPatch(enabled)
    {
        this.presentationState.beginPatch(enabled);
    }

    patchBankBypass(value)
    {
        this.presentationState.patchBankBypass(value);
    }

    patchRow(...args)
    {
        this.presentationState.patchRow(...args);
    }

    patchProcessor(...args)
    {
        this.presentationState.patchProcessor(...args);
    }

    removeRow(index)
    {
        this.presentationState.removeRow(index);
    }

    patchBank(...args)
    {
        this.presentationState.patchBank(...args);
    }

    patchGroupAction(enabled, active)
    {
        this.presentationState.patchAction("groupAction", enabled, active, null);
    }

    patchUngroupAction(enabled, active)
    {
        this.presentationState.patchAction("ungroupAction", enabled, active, null);
    }

    patchClearAction(enabled)
    {
        this.presentationState.patchAction("clearAction", enabled, false, null);
    }

    patchScopeAction(enabled, active, hasColor, red, green, blue, alpha)
    {
        this.presentationState.patchAction(
            "scopeAction",
            enabled,
            active,
            colorFromArguments(hasColor, red, green, blue, alpha)
        );
    }

    patchHistory(...args)
    {
        this.presentationState.patchHistory(...args);
    }

    endPresentationPatch()
    {
        this.interaction.layout();
        this.graphics.redraw();
    }

    destroy()
    {
        this.interaction.resetPointer();
        this.feedback.destroy();
    }
}

module.exports = {
    BankManagerControlCore: BankManagerControlCore
};
