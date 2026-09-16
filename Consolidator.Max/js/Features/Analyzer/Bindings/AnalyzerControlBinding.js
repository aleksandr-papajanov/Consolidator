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
            this.unsubscribers.push(presenter.subscribeSpectrum((spectrum, groupSpectrum) => {
                if (this.presentationActive && !this.batchSuspended)
                {
                    this.sendCurve("spectrum", spectrum);
                    this.sendGroupSpectrum(groupSpectrum, spectrum);
                }
            }, true));
        }
        if (presenter && typeof presenter.subscribeCurves === "function")
        {
            this.unsubscribers.push(presenter.subscribeCurves((curves, combinedCurve, allBanksCurve) => {
                if (this.presentationActive && !this.batchSuspended)
                {
                    (curves || []).forEach((curve) => {
                        this.sendCurve("curve", curve, curve.id);
                    });
                    this.sendCurve("combined", combinedCurve);
                    this.sendCurve("all_banks_curve", allBanksCurve);
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
        let roughGrid = presentation.roughGrid || {};
        let frequencies = roughGrid.frequencies || [];
        let gains = roughGrid.gains || [];
        this.send("rough_grid", [
            presentation.precisionRough ? 1 : 0,
            frequencies.length,
            gains.length
        ].concat(frequencies, gains));
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
        this.sendGroupSpectrum(
            this.presenter.groupSpectrum,
            this.presenter.spectrum
        );
        (this.presenter.curves || []).forEach((curve) => {
            this.sendCurve("curve", curve, curve.id);
        });
        this.sendCurve("combined", this.presenter.combinedCurve);
        this.sendCurve("all_banks_curve", this.presenter.overallCurve);
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
        if (name === "spectrum" || name === "group_spectrum")
        {
            args.push(Number(curve.fftSize), Number(curve.sampleRate));
        }
        args = args.concat(curve.values || []);
        this.send(name, args);
    }

    sendGroupSpectrum(groupSpectrum, sourceSpectrum)
    {
        if (groupSpectrum)
        {
            this.sendCurve("group_spectrum", groupSpectrum);
            return;
        }
        if (!sourceSpectrum)
        {
            return;
        }

        this.sendCurve("group_spectrum", {
            active: false,
            fftSize: sourceSpectrum.fftSize,
            sampleRate: sourceSpectrum.sampleRate,
            values: []
        });
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
