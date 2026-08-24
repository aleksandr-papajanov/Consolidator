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
    if (this.ready) {
        (this.options.parameters || []).forEach(function (parameter, index) {
            presentation.handles.push({
                id: index + 1,
                frequency: this.frequencyToX(
                    this.read(parameter.frequency, 1000)),
                gain: this.gainToY(this.read(parameter.gain, 0)),
                enabled: this.read(parameter.enabled, true),
                selected: index + 1 === this.selectedId,
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
    if (typeof parameter.setPosition === "function") {
        parameter.setPosition(frequency, gain, transactionId);
        return;
    }
    presentationBindingWrite(parameter.frequency, frequency, transactionId);
    presentationBindingWrite(parameter.gain, gain, transactionId);
};

AnalyzerPresenter.prototype.filterQChanged = function (id, delta) {
    var parameter = (this.options.parameters || [])[Number(id) - 1];
    if (!parameter || !parameter.q || !this.ready) {
        return;
    }
    var source = presentationBindingSource(parameter.q);
    var minimum = source.minimum !== undefined
        ? source.minimum : source.physicalMinimum !== undefined
            ? source.physicalMinimum : 0.01;
    var maximum = source.maximum !== undefined
        ? source.maximum : source.physicalMaximum !== undefined
            ? source.physicalMaximum : Number.POSITIVE_INFINITY;
    var current = Number(this.read(parameter.q, minimum));
    var next = Math.max(Number(minimum), Math.min(
        Number(maximum), current + Number(delta)));
    presentationBindingWrite(parameter.q, next);
};

AnalyzerPresenter.prototype.destroy = function () {
    this.unsubscribers.forEach(function (unsubscribe) {
        unsubscribe();
    });
    this.unsubscribers = [];
    PresentationObservable.prototype.destroy.call(this);
};
