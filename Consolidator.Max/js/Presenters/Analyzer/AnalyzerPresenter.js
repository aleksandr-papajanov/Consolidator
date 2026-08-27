const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { presentationBindingSource } = require("../Core/PresentationBinding.js");
const { presentationBindingValue } = require("../Core/PresentationBinding.js");
const { presentationBindingWrite } = require("../Core/PresentationBinding.js");
const { subscribePresentationBinding } = require("../Core/PresentationBinding.js");
const { AnalyzerPresentation } = require("./AnalyzerPresentation.js");

class AnalyzerPresenter extends PresentationObservable
{
    constructor(options)
    {
        super();
        this.options = options || {};
        this.ready = false;
        this.selectedId = 0;
        this.unsubscribers = [];
        let frequencyRange = this.options.frequencyRange || {};
        let gainRange = this.options.gainRange || {};
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
    
    subscribeParameters()
    {
        (this.options.parameters || []).forEach((parameter) => {
            [
                parameter.frequency,
                parameter.gain,
                parameter.q,
                parameter.enabled
            ].forEach((source) => {
                if (!source) {
                    return;
                }
                subscribePresentationBinding(source, () => {
                    if (this.ready) {
                        this.requestRebuild();
                    }
                }, this.unsubscribers);
            });
        });
    }
    
    subscribeStatus()
    {
        let statusSource = this.options.statusSource;
        if (!statusSource || typeof statusSource.subscribeStatus !== "function") {
            this.ready = true;
            return;
        }
        this.unsubscribers.push(statusSource.subscribeStatus((status) => {
            this.ready = Boolean(status && status.ready);
            if (!this.ready) {
                this.selectedId = 0;
            }
            this.requestRebuild();
        }, true));
    }
    
    connectSpectrum(protocol)
    {
        if (!protocol || typeof protocol.on !== "function") {
            return;
        }
        this.unsubscribers.push(protocol.on("fft", (args) => {
            let fftSize = Number(args[2]);
            let binCount = Math.floor(fftSize / 2) + 1;
            if (!isFinite(fftSize) || binCount <= 1 ||
                    args.length < 3 + binCount * 2) {
                return;
            }
            this.spectrum = {
                active: true,
                values: args.slice(3, 3 + binCount).map(Number)
            };
            this.referenceSpectrum = {
                active: true,
                values: args.slice(3 + binCount, 3 + binCount * 2).map(Number)
            };
            this.publishSpectrum();
        }));
    }
    
    subscribeSpectrum(callback, immediate)
    {
        this.spectrumListeners.push(callback);
        if (immediate && (this.spectrum || this.referenceSpectrum)) {
            callback(this.spectrum, this.referenceSpectrum);
        }
        return () => {
            this.spectrumListeners = this.spectrumListeners.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    publishSpectrum()
    {
        let listeners = this.spectrumListeners.slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](this.spectrum, this.referenceSpectrum);
        }
    }
    
    connectCurves(protocol, selector)
    {
        if (!protocol || typeof protocol.on !== "function") {
            return;
        }
        this.unsubscribers.push(protocol.on(selector || "equalizer_curves", (args) => {
            if (!args || Number(args[0]) !== 1 || args.length < 3) return;
            let filterCount = Number(args[2]);
            if (!isFinite(filterCount) || filterCount < 0 || filterCount > 32) return;
            let position = 3;
            let curves = [];
            for (let filterIndex = 0; filterIndex < filterCount; filterIndex++) {
                if (position + 257 > args.length) return;
                curves.push({
                    id: filterIndex + 1,
                    active: Number(args[position++]) !== 0,
                    values: args.slice(position, position + 256).map(Number)
                });
                position += 256;
            }
            if (position + 512 > args.length) return;
            this.curves = curves;
            this.combinedCurve = {
                active: Number(args[1]) !== 0,
                values: args.slice(position, position + 256).map(Number)
            };
            position += 256;
            this.allBanksCurve = {
                active: true,
                values: args.slice(position, position + 256).map(Number)
            };
            this.publishCurves();
        }));
    }
    
    subscribeCurves(callback, immediate)
    {
        this.curveListeners.push(callback);
        if (immediate && (this.curves.length || this.combinedCurve ||
                this.allBanksCurve)) {
            callback(this.curves, this.combinedCurve, this.allBanksCurve);
        }
        return () => {
            this.curveListeners = this.curveListeners.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    publishCurves()
    {
        let previousCurves = this.lastPublishedCurves;
        let changedCurves = this.curves.filter((curve) => {
            if (!previousCurves) return true;
            let previous = previousCurves.filter((candidate) => {
                return candidate.id === curve.id;
            })[0];
            if (!previous || previous.active !== curve.active ||
                    previous.values.length !== curve.values.length) return true;
            for (let index = 0; index < curve.values.length; index += 1) {
                if (previous.values[index] !== curve.values[index]) return true;
            }
            return false;
        });
        let combinedChanged = !this.sameCurve(
            this.lastPublishedCombined, this.combinedCurve);
        let allBanksChanged = !this.sameCurve(
            this.lastPublishedAllBanks, this.allBanksCurve);
        this.lastPublishedCurves = this.curves;
        this.lastPublishedCombined = this.combinedCurve;
        this.lastPublishedAllBanks = this.allBanksCurve;
        let listeners = this.curveListeners.slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](
                changedCurves,
                combinedChanged ? this.combinedCurve : null,
                allBanksChanged ? this.allBanksCurve : null);
        }
    }
    
    sameCurve(first, second)
    {
        if (!first || !second || first.active !== second.active ||
                first.values.length !== second.values.length) return false;
        for (let index = 0; index < first.values.length; index += 1) {
            if (first.values[index] !== second.values[index]) return false;
        }
        return true;
    }
    
    read(source, fallback)
    {
        return presentationBindingValue(source, fallback);
    }
    
    frequencyToX(value)
    {
        let minimum = Math.log(this.frequencyMinimum);
        return (Math.log(Math.max(this.frequencyMinimum, Number(value))) - minimum) /
            (Math.log(this.frequencyMaximum) - minimum);
    }
    
    xToFrequency(x)
    {
        let position = Math.max(0, Math.min(1, Number(x)));
        return Math.exp(Math.log(this.frequencyMinimum) + position *
            (Math.log(this.frequencyMaximum) - Math.log(this.frequencyMinimum)));
    }
    
    gainToY(value)
    {
        return Math.max(0, Math.min(1, 1 -
            (Number(value) - this.gainMinimum) /
            (this.gainMaximum - this.gainMinimum)));
    }
    
    yToGain(y)
    {
        let position = Math.max(0, Math.min(1, Number(y)));
        return this.gainMinimum + (1 - position) *
            (this.gainMaximum - this.gainMinimum);
    }
    
    rebuild()
    {
        let presentation = new AnalyzerPresentation();
        presentation.mode = this.options.mode || "equalizer";
        presentation.enabled = this.ready;
        presentation.spectrum = this.spectrum;
        presentation.referenceSpectrum = this.referenceSpectrum;
        presentation.combinedCurve = this.combinedCurve;
        presentation.allBanksCurve = this.allBanksCurve;
        presentation.curves = this.curves;
        if (this.ready) {
            (this.options.parameters || []).forEach((parameter, index) => {
                let frequencyMinimum = this.clampParameterValue(
                    parameter.frequency, this.frequencyMinimum);
                let frequencyMaximum = this.clampParameterValue(
                    parameter.frequency, this.frequencyMaximum);
                let gainMinimum = this.clampParameterValue(
                    parameter.gain, this.gainMinimum);
                let gainMaximum = this.clampParameterValue(
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
    }
    
    selectFilter(id)
    {
        let nextId = Math.floor(Number(id));
        let count = (this.options.parameters || []).length;
        this.selectedId = isFinite(nextId) && nextId >= 1 && nextId <= count
            ? nextId : 0;
        this.rebuild();
    }
    
    filterMoved(
        id, x, y, transactionId
    )
    {
        let parameter = (this.options.parameters || [])[Number(id) - 1];
        if (!parameter || !this.ready) {
            return;
        }
        let frequency = this.xToFrequency(x);
        let gain = this.yToGain(y);
        frequency = this.clampParameterValue(parameter.frequency, frequency);
        gain = this.clampParameterValue(parameter.gain, gain);
        if (typeof parameter.setPosition === "function") {
            parameter.setPosition(frequency, gain, transactionId);
            return;
        }
        presentationBindingWrite(parameter.frequency, frequency, transactionId);
        presentationBindingWrite(parameter.gain, gain, transactionId);
    }
    
    clampParameterValue(source, value)
    {
        let parameter = presentationBindingSource(source);
        let minimum = Number(parameter && parameter.minimum);
        if (!isFinite(minimum)) {
            minimum = Number(parameter && parameter.physicalMinimum);
        }
        let maximum = Number(parameter && parameter.maximum);
        if (!isFinite(maximum)) {
            maximum = Number(parameter && parameter.physicalMaximum);
        }
        let next = Number(value);
        if (isFinite(minimum)) {
            next = Math.max(minimum, next);
        }
        if (isFinite(maximum)) {
            next = Math.min(maximum, next);
        }
        return next;
    }
    
    filterQChanged(id, delta)
    {
        let parameter = (this.options.parameters || [])[Number(id) - 1];
        if (!parameter || !parameter.q || !this.ready) {
            return;
        }
        let source = presentationBindingSource(parameter.q);
        let minimum = Number(source.minimum);
        if (!isFinite(minimum)) {
            minimum = Number(source.physicalMinimum);
        }
        if (!isFinite(minimum)) {
            minimum = 0.01;
        }
        let maximum = Number(source.maximum);
        if (!isFinite(maximum)) {
            maximum = Number(source.physicalMaximum);
        }
        if (!isFinite(maximum)) {
            maximum = 10;
        }
        if (maximum < minimum) {
            return;
        }
        let current = Number(this.read(parameter.q));
        let change = Number(delta);
        if (!isFinite(current) || !isFinite(change) ||
                current < minimum || current > maximum) {
            return;
        }
        let next = Math.max(minimum, Math.min(maximum, current + change));
        presentationBindingWrite(parameter.q, next);
    }
    
    destroy()
    {
        this.unsubscribers.forEach((unsubscribe) => {
            unsubscribe();
        });
        this.unsubscribers = [];
        this.spectrumListeners = [];
        this.curveListeners = [];
        super.destroy();
    }
}


module.exports = {
    AnalyzerPresenter: AnalyzerPresenter
};
