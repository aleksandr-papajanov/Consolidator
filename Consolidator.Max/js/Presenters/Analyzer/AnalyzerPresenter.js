include("Project:/js/Presenters/Core/PresentationObservable.js");
include("Project:/js/Presenters/Core/PresentationBinding.js");
include("Project:/js/Presenters/Analyzer/AnalyzerPresentation.js");

function AnalyzerPresenter(options) {
    PresentationObservable.call(this);
    this.options = options || {};
    this.ready = false;
    this.selectedId = 0;
    this.unsubscribers = [];
    var frequencyRange = this.options.frequencyRange || {};
    var gainRange = this.options.gainRange || {};
    this.frequencyMinimum = frequencyRange.minimum === undefined
        ? 20 : Number(frequencyRange.minimum);
    this.frequencyMaximum = frequencyRange.maximum === undefined
        ? 20000 : Number(frequencyRange.maximum);
    this.gainMinimum = gainRange.minimum === undefined
        ? -24 : Number(gainRange.minimum);
    this.gainMaximum = gainRange.maximum === undefined
        ? 24 : Number(gainRange.maximum);
    this.spectrum = null;
    this.referenceSpectrum = null;
    this.curves = [];
    this.combinedCurve = null;
    this.allBanksCurve = null;
    this.spectrumListeners = [];
    this.curveListeners = [];
    this.lastPublishedCurves = null;
    this.lastPublishedCombined = null;
    this.lastPublishedAllBanks = null;
    this.subscribeParameters();
    this.subscribeStatus();
    this.rebuild();
}

AnalyzerPresenter.prototype = Object.create(PresentationObservable.prototype);
AnalyzerPresenter.prototype.constructor = AnalyzerPresenter;

AnalyzerPresenter.prototype.subscribeParameters = function () {
    var self = this;
    (this.options.parameters || []).forEach(function (parameter) {
        [
            parameter.frequency,
            parameter.gain,
            parameter.q,
            parameter.enabled
        ].forEach(function (source) {
            if (!source) {
                return;
            }
            subscribePresentationBinding(source, function () {
                if (self.ready) {
                    self.rebuild();
                }
            }, self.unsubscribers);
        });
    });
};

AnalyzerPresenter.prototype.subscribeStatus = function () {
    var self = this;
    var statusSource = this.options.statusSource;
    if (!statusSource || typeof statusSource.subscribeStatus !== "function") {
        this.ready = true;
        return;
    }
    this.unsubscribers.push(statusSource.subscribeStatus(function (status) {
        self.ready = Boolean(status && status.ready);
        if (!self.ready) {
            self.selectedId = 0;
        }
        self.rebuild();
    }, true));
};

AnalyzerPresenter.prototype.connectSpectrum = function (protocol) {
    if (!protocol || typeof protocol.on !== "function") {
        return;
    }
    var self = this;
    this.unsubscribers.push(protocol.on("fft", function (args) {
        var fftSize = Number(args[2]);
        var binCount = Math.floor(fftSize / 2) + 1;
        if (!isFinite(fftSize) || binCount <= 1 ||
                args.length < 3 + binCount * 2) {
            return;
        }
        self.spectrum = {
            active: true,
            values: args.slice(3, 3 + binCount).map(Number)
        };
        self.referenceSpectrum = {
            active: true,
            values: args.slice(3 + binCount, 3 + binCount * 2).map(Number)
        };
        self.publishSpectrum();
    }));
};

AnalyzerPresenter.prototype.subscribeSpectrum = function (callback, immediate) {
    this.spectrumListeners.push(callback);
    if (immediate && (this.spectrum || this.referenceSpectrum)) {
        callback(this.spectrum, this.referenceSpectrum);
    }
    var self = this;
    return function () {
        self.spectrumListeners = self.spectrumListeners.filter(
            function (listener) { return listener !== callback; });
    };
};

AnalyzerPresenter.prototype.publishSpectrum = function () {
    var listeners = this.spectrumListeners.slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](this.spectrum, this.referenceSpectrum);
    }
};

AnalyzerPresenter.prototype.connectCurves = function (protocol, selector) {
    if (!protocol || typeof protocol.on !== "function") {
        return;
    }
    var self = this;
    this.unsubscribers.push(protocol.on(selector || "equalizer_curves", function (args) {
        if (!args || Number(args[0]) !== 1 || args.length < 3) return;
        var filterCount = Number(args[2]);
        if (!isFinite(filterCount) || filterCount < 0 || filterCount > 32) return;
        var position = 3;
        var curves = [];
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            if (position + 257 > args.length) return;
            curves.push({
                id: filterIndex + 1,
                active: Number(args[position++]) !== 0,
                values: args.slice(position, position + 256).map(Number)
            });
            position += 256;
        }
        if (position + 512 > args.length) return;
        self.curves = curves;
        self.combinedCurve = {
            active: Number(args[1]) !== 0,
            values: args.slice(position, position + 256).map(Number)
        };
        position += 256;
        self.allBanksCurve = {
            active: true,
            values: args.slice(position, position + 256).map(Number)
        };
        self.publishCurves();
    }));
};

AnalyzerPresenter.prototype.subscribeCurves = function (callback, immediate) {
    this.curveListeners.push(callback);
    if (immediate && (this.curves.length || this.combinedCurve ||
            this.allBanksCurve)) {
        callback(this.curves, this.combinedCurve, this.allBanksCurve);
    }
    var self = this;
    return function () {
        self.curveListeners = self.curveListeners.filter(
            function (listener) { return listener !== callback; });
    };
};

AnalyzerPresenter.prototype.publishCurves = function () {
    var previousCurves = this.lastPublishedCurves;
    var changedCurves = this.curves.filter(function (curve) {
        if (!previousCurves) return true;
        var previous = previousCurves.filter(function (candidate) {
            return candidate.id === curve.id;
        })[0];
        if (!previous || previous.active !== curve.active ||
                previous.values.length !== curve.values.length) return true;
        for (var index = 0; index < curve.values.length; index += 1) {
            if (previous.values[index] !== curve.values[index]) return true;
        }
        return false;
    });
    var combinedChanged = !this.sameCurve(
        this.lastPublishedCombined, this.combinedCurve);
    var allBanksChanged = !this.sameCurve(
        this.lastPublishedAllBanks, this.allBanksCurve);
    this.lastPublishedCurves = this.curves;
    this.lastPublishedCombined = this.combinedCurve;
    this.lastPublishedAllBanks = this.allBanksCurve;
    var listeners = this.curveListeners.slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](
            changedCurves,
            combinedChanged ? this.combinedCurve : null,
            allBanksChanged ? this.allBanksCurve : null);
    }
};

AnalyzerPresenter.prototype.sameCurve = function (first, second) {
    if (!first || !second || first.active !== second.active ||
            first.values.length !== second.values.length) return false;
    for (var index = 0; index < first.values.length; index += 1) {
        if (first.values[index] !== second.values[index]) return false;
    }
    return true;
};

AnalyzerPresenter.prototype.read = function (source, fallback) {
    return presentationBindingValue(source, fallback);
};

AnalyzerPresenter.prototype.frequencyToX = function (value) {
    var minimum = Math.log(this.frequencyMinimum);
    return (Math.log(Math.max(this.frequencyMinimum, Number(value))) - minimum) /
        (Math.log(this.frequencyMaximum) - minimum);
};

AnalyzerPresenter.prototype.xToFrequency = function (x) {
    var position = Math.max(0, Math.min(1, Number(x)));
    return Math.exp(Math.log(this.frequencyMinimum) + position *
        (Math.log(this.frequencyMaximum) - Math.log(this.frequencyMinimum)));
};

AnalyzerPresenter.prototype.gainToY = function (value) {
    return Math.max(0, Math.min(1, 1 -
        (Number(value) - this.gainMinimum) /
        (this.gainMaximum - this.gainMinimum)));
};

AnalyzerPresenter.prototype.yToGain = function (y) {
    var position = Math.max(0, Math.min(1, Number(y)));
    return this.gainMinimum + (1 - position) *
        (this.gainMaximum - this.gainMinimum);
};

AnalyzerPresenter.prototype.rebuild = function () {
    var presentation = new AnalyzerPresentation();
    presentation.mode = this.options.mode || "equalizer";
    presentation.enabled = this.ready;
    presentation.spectrum = this.spectrum;
    presentation.referenceSpectrum = this.referenceSpectrum;
    presentation.combinedCurve = this.combinedCurve;
    presentation.allBanksCurve = this.allBanksCurve;
    presentation.curves = this.curves;
    if (this.ready) {
        (this.options.parameters || []).forEach(function (parameter, index) {
            var frequencyMinimum = this.clampParameterValue(
                parameter.frequency, this.frequencyMinimum);
            var frequencyMaximum = this.clampParameterValue(
                parameter.frequency, this.frequencyMaximum);
            var gainMinimum = this.clampParameterValue(
                parameter.gain, this.gainMinimum);
            var gainMaximum = this.clampParameterValue(
                parameter.gain, this.gainMaximum);
            presentation.handles.push({
                id: index + 1,
                frequency: this.frequencyToX(
                    this.read(parameter.frequency, 1000)),
                gain: this.gainToY(this.read(parameter.gain, 0)),
                enabled: this.read(parameter.enabled, true),
                selected: index + 1 === this.selectedId,
                xMinimum: this.frequencyToX(frequencyMinimum),
                xMaximum: this.frequencyToX(frequencyMaximum),
                yMinimum: this.gainToY(gainMaximum),
                yMaximum: this.gainToY(gainMinimum),
                capabilities: {
                    frequency: Boolean(parameter.frequency),
                    gain: Boolean(parameter.gain),
                    q: Boolean(parameter.q)
                }
            });
        }, this);
    }
    this.publish(presentation);
};

AnalyzerPresenter.prototype.selectFilter = function (id) {
    var nextId = Math.floor(Number(id));
    var count = (this.options.parameters || []).length;
    this.selectedId = isFinite(nextId) && nextId >= 1 && nextId <= count
        ? nextId : 0;
    this.rebuild();
};

AnalyzerPresenter.prototype.filterMoved = function (
    id, x, y, transactionId
) {
    var parameter = (this.options.parameters || [])[Number(id) - 1];
    if (!parameter || !this.ready) {
        return;
    }
    var frequency = this.xToFrequency(x);
    var gain = this.yToGain(y);
    frequency = this.clampParameterValue(parameter.frequency, frequency);
    gain = this.clampParameterValue(parameter.gain, gain);
    if (typeof parameter.setPosition === "function") {
        parameter.setPosition(frequency, gain, transactionId);
        return;
    }
    presentationBindingWrite(parameter.frequency, frequency, transactionId);
    presentationBindingWrite(parameter.gain, gain, transactionId);
};

AnalyzerPresenter.prototype.clampParameterValue = function (source, value) {
    var parameter = presentationBindingSource(source);
    var minimum = Number(parameter && parameter.minimum);
    if (!isFinite(minimum)) {
        minimum = Number(parameter && parameter.physicalMinimum);
    }
    var maximum = Number(parameter && parameter.maximum);
    if (!isFinite(maximum)) {
        maximum = Number(parameter && parameter.physicalMaximum);
    }
    var next = Number(value);
    if (isFinite(minimum)) {
        next = Math.max(minimum, next);
    }
    if (isFinite(maximum)) {
        next = Math.min(maximum, next);
    }
    return next;
};

AnalyzerPresenter.prototype.filterQChanged = function (id, delta) {
    var parameter = (this.options.parameters || [])[Number(id) - 1];
    if (!parameter || !parameter.q || !this.ready) {
        return;
    }
    var source = presentationBindingSource(parameter.q);
    var minimum = Number(source.minimum);
    if (!isFinite(minimum)) {
        minimum = Number(source.physicalMinimum);
    }
    if (!isFinite(minimum)) {
        minimum = 0.01;
    }
    var maximum = Number(source.maximum);
    if (!isFinite(maximum)) {
        maximum = Number(source.physicalMaximum);
    }
    if (!isFinite(maximum)) {
        maximum = 10;
    }
    if (maximum < minimum) {
        return;
    }
    var current = Number(this.read(parameter.q));
    var change = Number(delta);
    if (!isFinite(current) || !isFinite(change) ||
            current < minimum || current > maximum) {
        return;
    }
    var next = Math.max(minimum, Math.min(maximum, current + change));
    presentationBindingWrite(parameter.q, next);
};

AnalyzerPresenter.prototype.destroy = function () {
    this.unsubscribers.forEach(function (unsubscribe) {
        unsubscribe();
    });
    this.unsubscribers = [];
    this.spectrumListeners = [];
    this.curveListeners = [];
    PresentationObservable.prototype.destroy.call(this);
};
