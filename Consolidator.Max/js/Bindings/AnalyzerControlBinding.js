const { ControlBinding } = require("./ControlBinding.js");

class AnalyzerControlBinding extends ControlBinding
{
    constructor(controller, presenter, sendMessage, transactions)
    {
        super(presenter, sendMessage);
        this.controller = controller;
        this.transactions = transactions;
        this.activeTransactionId = null;
        this.transactionReady = false;
        this.pendingEnd = false;
        this.lastMove = null;
        this.connectPresentation();
        if (presenter && typeof presenter.subscribeSpectrum === "function") {
            this.unsubscribers.push(presenter.subscribeSpectrum((
                spectrum, referenceSpectrum
            ) => {
                if (!this.presentationActive || this.batchSuspended) {
                    return;
                }
                this.sendCurve("spectrum", spectrum);
                this.sendCurve("reference_spectrum", referenceSpectrum);
            }, true));
        }
        if (presenter && typeof presenter.subscribeCurves === "function") {
            this.unsubscribers.push(presenter.subscribeCurves((
                curves, combinedCurve, allBanksCurve
            ) => {
                if (!this.presentationActive || this.batchSuspended) {
                    return;
                }
                (curves || []).forEach((curve) => {
                    this.sendCurve("curve", curve, curve.id);
                });
                this.sendCurve("combined", combinedCurve);
                this.sendCurve("all_banks", allBanksCurve);
            }, true));
        }
    }
    
    applyPresentation(presentation)
    {
        this.send("presentation_begin", [
            presentation.mode || "equalizer",
            presentation.enabled ? 1 : 0,
            presentation.parameterRevision || 0,
            presentation.viewKey || ""
        ]);
        (presentation.handles || []).forEach((handle) => {
            let capabilities = handle.capabilities || {};
            this.send("handle", [
                handle.id,
                handle.frequency,
                handle.gain,
                handle.enabled ? 1 : 0,
                capabilities.frequency ? 1 : 0,
                capabilities.gain ? 1 : 0,
                capabilities.q ? 1 : 0,
                handle.selected ? 1 : 0,
                handle.xMinimum,
                handle.xMaximum,
                handle.yMinimum,
                handle.yMaximum
            ]);
        }, this);
        this.send("presentation_end");
    }
    
    refreshPresentation()
    {
        super.refreshPresentation();
        if (!this.presenter) {
            return;
        }
        this.sendCurve("spectrum", this.presenter.spectrum);
        this.sendCurve("reference_spectrum", this.presenter.referenceSpectrum);
        (this.presenter.curves || []).forEach((curve) => {
            this.sendCurve("curve", curve, curve.id);
        }, this);
        this.sendCurve("combined", this.presenter.combinedCurve);
        this.sendCurve("all_banks", this.presenter.allBanksCurve);
    }
    
    sendCurve(name, curve, id)
    {
        if (!curve) {
            return;
        }
        let args = id === undefined ? [] : [id];
        args.push(curve.active === false ? 0 : 1);
        args = args.concat(curve.values || []);
        this.send(name, args);
    }
    
    handleIntent(name, values)
    {
        if (name === "gestureBegan") {
            this.beginTransaction();
            return;
        }
        if (name === "gestureEnded") {
            if (this.activeTransactionId !== null && !this.transactionReady) {
                this.pendingEnd = true;
            }
            else {
                this.commitAndFinish();
            }
            return;
        }
        if (name === "filterMoved") {
            this.lastMove = values.slice(0);
        }
        this.controller.handle(name, values, this.activeTransactionId);
    }
    
    beginTransaction()
    {
        if (!this.transactions || this.activeTransactionId !== null) {
            return;
        }
        this.transactionReady = false;
        this.pendingEnd = false;
        this.activeTransactionId = this.transactions.begin((id, response) => {
            this.completeTransactionBegin(id, response);
        });
    }
    
    completeTransactionBegin(
        id, response
    )
    {
        if (!this.transactions || this.activeTransactionId !== id) {
            return;
        }
        if (!response || response.status !== "accepted") {
            if (this.controller && this.controller.presenter) {
                this.controller.presenter.curvePreview = {};
                this.controller.presenter.requestRebuild();
            }
            this.send("transactionRejected");
            this.clearTransaction();
            return;
        }
        this.transactionReady = true;
        if (this.pendingEnd) {
            this.commitAndFinish();
        }
    }

    commitAndFinish()
    {
        if (!this.lastMove || !this.controller ||
                this.activeTransactionId === null) {
            this.finishTransaction();
            return;
        }
        let transactionId = this.activeTransactionId;
        this.controller.handle(
            "filterCommit",
            [this.lastMove[0]],
            transactionId,
            (response) => {
                if (!response || response.error || response.status !== "accepted") {
                    if (this.controller && this.controller.presenter) {
                        this.controller.presenter.curvePreview = {};
                        this.controller.presenter.requestRebuild();
                    }
                    this.send("transactionRejected");
                }
                this.finishTransaction();
            });
    }
    
    finishTransaction()
    {
        if (this.transactions && this.activeTransactionId !== null &&
                this.transactionReady) {
            this.transactions.end(this.activeTransactionId);
        }
        this.clearTransaction();
    }
    
    clearTransaction()
    {
        this.activeTransactionId = null;
        this.transactionReady = false;
        this.pendingEnd = false;
        this.lastMove = null;
    }
    
    destroy()
    {
        this.finishTransaction();
        this.transactions = null;
        this.controller = null;
        super.destroy();
    }
}

module.exports = {
    AnalyzerControlBinding: AnalyzerControlBinding
};
