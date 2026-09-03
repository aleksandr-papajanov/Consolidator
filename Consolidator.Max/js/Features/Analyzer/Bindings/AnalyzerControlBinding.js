const { AnalyzerGestureSession } = require("./AnalyzerGestureSession.js");
const { ControlBinding } = require("../../../Shared/Bindings/ControlBinding.js");

class AnalyzerControlBinding extends ControlBinding
{
    constructor(controller, presenter, sendMessage, transactions)
    {
        super(presenter, sendMessage);
        this.gesture = new AnalyzerGestureSession(
            controller,
            transactions,
            (selector) => this.send(selector)
        );
        this.connectPresentation();
        if (presenter && typeof presenter.subscribeSpectrum === "function")
        {
            this.unsubscribers.push(presenter.subscribeSpectrum((spectrum, reference) => {
                if (this.presentationActive && !this.batchSuspended)
                {
                    this.sendCurve("spectrum", spectrum);
                    this.sendCurve("reference_spectrum", reference);
                }
            }, true));
        }
        if (presenter && typeof presenter.subscribeCurves === "function")
        {
            this.unsubscribers.push(presenter.subscribeCurves((curves, combinedCurve) => {
                if (this.presentationActive && !this.batchSuspended)
                {
                    (curves || []).forEach((curve) => {
                        this.sendCurve("curve", curve, curve.id);
                    });
                    this.sendCurve("combined", combinedCurve);
                }
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
        const color = presentation.scopeColor;
        this.send("scope", [
            presentation.scopeActive ? 1 : 0,
            color && color.length >= 4 ? 1 : 0
        ].concat(color && color.length >= 4 ? color : [0, 0, 0, 0]));
        (presentation.handles || []).forEach((handle) => {
            const capabilities = handle.capabilities || {};
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
        });
        this.send("presentation_end");
    }

    refreshPresentation()
    {
        super.refreshPresentation();
        if (!this.presenter)
        {
            return;
        }
        this.sendCurve("spectrum", this.presenter.spectrum);
        this.sendCurve("reference_spectrum", this.presenter.referenceSpectrum);
        (this.presenter.curves || []).forEach((curve) => {
            this.sendCurve("curve", curve, curve.id);
        });
        this.sendCurve("combined", this.presenter.combinedCurve);
    }

    suspend()
    {
        super.suspend();
        this.gesture.cancel();
    }

    sendCurve(name, curve, id)
    {
        if (!curve)
        {
            return;
        }
        let args = id === undefined ? [] : [id];
        args.push(curve.active === false ? 0 : 1);
        args = args.concat(curve.values || []);
        this.send(name, args);
    }

    handleIntent(name, values)
    {
        this.gesture.handleIntent(name, values);
    }

    destroy()
    {
        this.gesture.destroy();
        this.gesture = null;
        super.destroy();
    }
}

module.exports = {
    AnalyzerControlBinding: AnalyzerControlBinding
};
