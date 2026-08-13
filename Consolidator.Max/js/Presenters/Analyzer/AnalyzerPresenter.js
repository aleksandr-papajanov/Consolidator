include("../Core/PresentationObservable.js");
include("../Core/PresentationBinding.js");
include("AnalyzerPresentation.js");

function AnalyzerPresenter(options) {
    PresentationObservable.call(this);
    this.options = options || {};
    this.eventListeners = {};
    this.unsubscribers = [];
    var frequencyRange = this.options.frequencyRange || {};
    var gainRange = this.options.gainRange || {};
    this.frequencyMinimum = frequencyRange.minimum === undefined
        ? 20 : frequencyRange.minimum;
    this.frequencyMaximum = frequencyRange.maximum === undefined
        ? 20000 : frequencyRange.maximum;
    this.gainMinimum = gainRange.minimum === undefined
        ? -24 : gainRange.minimum;
    this.gainMaximum = gainRange.maximum === undefined
        ? 24 : gainRange.maximum;
    var spectrumRange = this.options.spectrumRange || {};
    this.spectrumMinimum = spectrumRange.minimum === undefined
        ? -120 : spectrumRange.minimum;
    this.spectrumMaximum = spectrumRange.maximum === undefined
        ? 0 : spectrumRange.maximum;
    this.selectedId = 0;
    this.subscribeSources();
    this.rebuild();
}

AnalyzerPresenter.prototype = Object.create(PresentationObservable.prototype);
AnalyzerPresenter.prototype.constructor = AnalyzerPresenter;

AnalyzerPresenter.prototype.on = function (name, callback) {
    if (!this.eventListeners[name]) this.eventListeners[name] = [];
    this.eventListeners[name].push(callback);
    var self = this;
    return function () {
        self.eventListeners[name] = (self.eventListeners[name] || []).filter(
            function (item) { return item !== callback; });
    };
};

AnalyzerPresenter.prototype.emit = function (name, payload) {
    (this.eventListeners[name] || []).slice().forEach(function (callback) {
        callback(payload);
    });
};

AnalyzerPresenter.prototype.subscribeSources = function () {
    var self = this;
    var sources = [
        this.options.spectrum,
        this.options.referenceSpectrum,
        this.options.differenceSpectrum,
        this.options.combined,
        this.options.enabled
    ];
    (this.options.curves || []).forEach(function (curve) { sources.push(curve); });
    (this.options.parameters || []).forEach(function (parameter) {
        sources.push(parameter.frequency, parameter.gain, parameter.q,
            parameter.enabled);
    });
    sources.forEach(function (source) {
        if (!source) return;
        subscribePresentationBinding(source, function () { self.rebuild(); },
            self.unsubscribers);
    });
};

AnalyzerPresenter.prototype.read = function (source, fallback) {
    return presentationBindingValue(source, fallback);
};

AnalyzerPresenter.prototype.frequencyToX = function (value) {
    var min = Math.log(this.frequencyMinimum);
    return (Math.log(Math.max(this.frequencyMinimum, Number(value))) - min) /
        (Math.log(this.frequencyMaximum) - min);
};

AnalyzerPresenter.prototype.xToFrequency = function (x) {
    return Math.exp(Math.log(this.frequencyMinimum) +
        Math.max(0, Math.min(1, x)) *
        (Math.log(this.frequencyMaximum) - Math.log(this.frequencyMinimum)));
};

AnalyzerPresenter.prototype.gainToY = function (value) {
    return this.rangeToY(value, this.gainMinimum, this.gainMaximum);
};

AnalyzerPresenter.prototype.rangeToY = function (value, minimum, maximum) {
    return Math.max(0, Math.min(1, 1 - (Number(value) - minimum) /
        (maximum - minimum)));
};

AnalyzerPresenter.prototype.yToGain = function (y) {
    return this.gainMinimum + (1 - Math.max(0, Math.min(1, y))) *
        (this.gainMaximum - this.gainMinimum);
};

AnalyzerPresenter.prototype.curve = function (id, source) {
    var value = this.read(source, null);
    return {
        id: id,
        active: value && value.active !== undefined ? value.active : true,
        values: value && value.values ? value.values.map(function (point) {
            return this.gainToY(point);
        }, this) : []
    };
};

AnalyzerPresenter.prototype.spectrum = function (source, minimum, maximum) {
    var value = this.read(source, null);
    if (!value || !value.values) return null;
    return {
        values: value.values.map(function (point) {
            return this.rangeToY(point, minimum, maximum);
        }, this)
    };
};

AnalyzerPresenter.prototype.rebuild = function () {
    var presentation = new AnalyzerPresentation();
    presentation.enabled = this.read(this.options.enabled, true);
    presentation.mode = this.options.mode || "equalizer";
    presentation.spectrum = this.spectrum(
        this.options.spectrum, this.spectrumMinimum, this.spectrumMaximum);
    presentation.referenceSpectrum = this.spectrum(
        this.options.referenceSpectrum, this.spectrumMinimum, this.spectrumMaximum);
    presentation.differenceSpectrum = this.spectrum(
        this.options.differenceSpectrum, this.gainMinimum, this.gainMaximum);
    (this.options.curves || []).forEach(function (curve, index) {
        presentation.curves.push(this.curve(index + 1, curve));
    }, this);
    presentation.combinedCurve = this.curve(0, this.options.combined);
    (this.options.parameters || []).forEach(function (parameter, index) {
        presentation.handles.push({
            id: index + 1,
            frequency: this.frequencyToX(this.read(parameter.frequency, 1000)),
            gain: this.gainToY(this.read(parameter.gain, 0)),
            enabled: this.read(parameter.enabled, true),
            selected: index + 1 === this.selectedId,
            capabilities: {
                frequency: !!parameter.frequency,
                gain: !!parameter.gain,
                q: !!parameter.q
            }
        });
    }, this);
    this.publish(presentation);
};

AnalyzerPresenter.prototype.selectFilter = function (id) {
    var nextId = Number(id);
    var count = (this.options.parameters || []).length;
    this.selectedId = isFinite(nextId) && nextId >= 1 && nextId <= count
        ? Math.floor(nextId) : 0;
    this.rebuild();
    this.emit("filterSelected", this.selectedId);
};

AnalyzerPresenter.prototype.beginGesture = function (id) {
    this.emit("gestureBegan", { id: Number(id) });
};

AnalyzerPresenter.prototype.endGesture = function (id) {
    this.emit("gestureEnded", { id: Number(id) });
};

AnalyzerPresenter.prototype.filterMoved = function (id, x, y) {
    var parameter = (this.options.parameters || [])[Number(id) - 1];
    if (!parameter) return;
    var frequency = this.xToFrequency(x);
    var gain = this.yToGain(y);
    if (parameter.setPosition) {
        parameter.setPosition(frequency, gain);
    }
    else {
        if (parameter.frequency) presentationBindingWrite(parameter.frequency,
            frequency);
        if (parameter.gain) presentationBindingWrite(parameter.gain, gain);
    }
    this.emit("filterMoved", { id: Number(id), x: x, y: y });
};

AnalyzerPresenter.prototype.filterQChanged = function (id, delta) {
    var parameter = (this.options.parameters || [])[Number(id) - 1];
    if (!parameter || !parameter.q) return;
    var source = presentationBindingSource(parameter.q);
    var minimum = source && source.minimum !== undefined
        ? source.minimum : source && source.physicalMinimum !== undefined
            ? source.physicalMinimum : 0.01;
    var maximum = source && source.maximum !== undefined
        ? source.maximum : source && source.physicalMaximum !== undefined
            ? source.physicalMaximum : Number.POSITIVE_INFINITY;
    var current = Number(this.read(parameter.q, minimum));
    var next = current + Number(delta);
    next = Math.max(Number(minimum), Math.min(Number(maximum), next));
    presentationBindingWrite(parameter.q, next);
};

AnalyzerPresenter.prototype.destroy = function () {
    this.unsubscribers.forEach(function (unsubscribe) { unsubscribe(); });
    this.unsubscribers = [];
    this.eventListeners = {};
    PresentationObservable.prototype.destroy.call(this);
};
