const { ControlBinding } = require("./ControlBinding.js");

class DialControlBinding extends ControlBinding
{
    constructor(presenter, sendMessage, transactions)
    {
        super(presenter, sendMessage);
        this.transactions = transactions;
        this.activeTransactionId = null;
        this.transactionReady = false;
        this.pendingValue = null;
        this.pendingEnd = null;
        this.connectPresentation();
    }
    
    applyPresentation(presentation)
    {
        if (this.hasPresentation && !this.requiresFullPresentation(presentation)) {
            this.send("presentation_begin");
            (presentation.rings || []).forEach((ring, index) => {
                let previous = this.presentation.rings[index];
                if (previous.value !== ring.value) {
                    this.send("set", [index, ring.value]);
                }
            }, this);
            this.send("presentation_end");
            this.presentation = presentation;
            return;
        }
        this.send("presentation_begin");
        this.send("enabled", [presentation.enabled ? 1 : 0]);
        this.send("active", [presentation.active ? 1 : 0]);
        this.send("activeIndex", [presentation.activeIndex || 0]);
        this.send("displayIndex", [presentation.displayIndex || 0]);
        this.send("ringCount", [(presentation.rings || []).length]);
        (presentation.rings || []).forEach((ring, index) => {
            this.send("limits", [index, ring.minimum, ring.maximum]);
            this.send("set", [index, ring.value]);
        }, this);
        this.send("presentation_end");
        this.presentation = presentation;
        this.hasPresentation = true;
    }
    
    requiresFullPresentation(presentation)
    {
        let previous = this.presentation || {};
        if (Boolean(previous.enabled) !== Boolean(presentation.enabled) ||
                Boolean(previous.active) !== Boolean(presentation.active) ||
                Number(previous.activeIndex) !== Number(presentation.activeIndex) ||
                Number(previous.displayIndex) !== Number(presentation.displayIndex)) {
            return true;
        }
        let previousRings = previous.rings || [];
        let rings = presentation.rings || [];
        if (previousRings.length !== rings.length) return true;
        for (let index = 0; index < rings.length; index += 1) {
            if (previousRings[index].minimum !== rings[index].minimum ||
                    previousRings[index].maximum !== rings[index].maximum ||
                    !this.sameVisualization(previousRings[index].visualization,
                        rings[index].visualization) ||
                    !this.sameColor(previousRings[index].color,
                        rings[index].color) ||
                    !this.sameDisplay(previousRings[index].display,
                        rings[index].display)) {
                return true;
            }
        }
        return false;
    }
    
    sameVisualization(first, second)
    {
        if (first === second) return true;
        if (!first || !second || first.type !== second.type) return false;
        if (first.type === "level") {
            return first.peak === second.peak && first.smoothed === second.smoothed;
        }
        return first.value === second.value;
    }
    
    sameColor(first, second)
    {
        if (first === second) return true;
        if (!first || !second || first.length !== second.length) return false;
        for (let index = 0; index < first.length; index += 1) {
            if (first[index] !== second[index]) return false;
        }
        return true;
    }
    
    sameDisplay(first, second)
    {
        first = first || {};
        second = second || {};
        return first.value === second.value;
    }
    
    handleIntent(name, values)
    {
        switch (name) {
        case "valueChanged":
            if (this.activeTransactionId !== null && !this.transactionReady) {
                this.pendingValue = [values[0], values[1]];
                break;
            }
            this.presenter.setValue(
                values[0],
                values[1],
                this.activeTransactionId);
            break;
        case "reset":
            this.presenter.resetValue(values[0]);
            break;
        case "gestureBegan":
            if (this.transactions) {
                this.transactionReady = false;
                this.pendingValue = null;
                this.pendingEnd = null;
                this.activeTransactionId = this.transactions.begin(
                    this.handleTransactionBegin.bind(this));
            }
            this.presenter.beginGesture(
                values[0],
                this.activeTransactionId);
            break;
        case "gestureEnded":
            if (this.transactions && this.activeTransactionId !== null) {
                if (this.transactionReady) {
                    this.presenter.endGesture(values[0], this.activeTransactionId);
                    this.finishTransaction();
                }
                else {
                    this.pendingEnd = values[0];
                }
            }
            else {
                this.presenter.endGesture(values[0], null);
            }
            break;
        }
    }
    
    handleTransactionBegin(id, response)
    {
        if (!this.transactions || this.activeTransactionId !== id) {
            return;
        }
        if (!response || response.status !== "accepted") {
            this.send("transactionRejected");
            this.clearTransaction();
            return;
        }
    
        this.transactionReady = true;
        if (this.pendingValue) {
            this.presenter.setValue(
                this.pendingValue[0],
                this.pendingValue[1],
                id);
            this.pendingValue = null;
        }
        if (this.pendingEnd !== null) {
            this.presenter.endGesture(this.pendingEnd, id);
            this.finishTransaction();
        }
    }
    
    finishTransaction()
    {
        this.transactions.end(this.activeTransactionId);
        this.clearTransaction();
    }
    
    clearTransaction()
    {
        this.activeTransactionId = null;
        this.transactionReady = false;
        this.pendingValue = null;
        this.pendingEnd = null;
    }
    
    destroy()
    {
        if (this.transactions && this.activeTransactionId !== null &&
                this.transactionReady) {
            this.transactions.end(this.activeTransactionId);
        }
        this.clearTransaction();
        this.transactions = null;
        super.destroy();
    }
}

module.exports = {
    DialControlBinding: DialControlBinding
};
