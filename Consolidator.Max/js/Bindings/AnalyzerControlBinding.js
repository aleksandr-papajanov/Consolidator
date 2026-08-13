include("ControlBinding.js");

function AnalyzerControlBinding(controller, presenter, sendMessage) {
    ControlBinding.call(this, presenter, sendMessage);
    this.controller = controller;
    this.connectPresentation();
}

AnalyzerControlBinding.prototype = Object.create(ControlBinding.prototype);
AnalyzerControlBinding.prototype.constructor = AnalyzerControlBinding;

AnalyzerControlBinding.prototype.applyPresentation = function (presentation) {
    this.send("presentation_begin", [
        presentation.mode || "equalizer",
        presentation.enabled ? 1 : 0
    ]);
    this.sendCurve("spectrum", presentation.spectrum);
    this.sendCurve("reference_spectrum", presentation.referenceSpectrum);
    this.sendCurve("difference_spectrum", presentation.differenceSpectrum);
    (presentation.curves || []).forEach(function (curve) {
        this.sendCurve("curve", curve, curve.id);
    }, this);
    this.sendCurve("combined", presentation.combinedCurve);
    (presentation.handles || []).forEach(function (handle) {
        var capabilities = handle.capabilities || {};
        this.send("handle", [
            handle.id,
            handle.frequency,
            handle.gain,
            handle.enabled ? 1 : 0,
            capabilities.frequency ? 1 : 0,
            capabilities.gain ? 1 : 0,
            capabilities.q ? 1 : 0,
            handle.selected ? 1 : 0
        ]);
    }, this);
    this.send("presentation_end");
};

AnalyzerControlBinding.prototype.sendCurve = function (name, curve, id) {
    if (!curve) {
        return;
    }
    var args = id === undefined ? [] : [id];
    args.push(curve.active === false ? 0 : 1);
    args = args.concat(curve.values || []);
    this.send(name, args);
};

AnalyzerControlBinding.prototype.handleIntent = function (name, values) {
    this.controller.handle(name, values);
};
