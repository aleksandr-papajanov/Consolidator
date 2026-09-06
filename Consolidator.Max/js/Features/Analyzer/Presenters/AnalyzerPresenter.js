const { PresentationObservable } = require(
    "../../../Shared/Presenters/PresentationObservable.js");
const { presentationBindingValue } = require(
    "../../../Shared/Presenters/PresentationBinding.js");
const { subscribePresentationBinding } = require(
    "../../../Shared/Presenters/PresentationBinding.js");
const { AnalyzerPresentation } = require("./AnalyzerPresentation.js");
const { AnalyzerCurveCalculator } = require("./AnalyzerCurveCalculator.js");
const { AnalyzerCurvePublisher } = require("./AnalyzerCurvePublisher.js");
const { AnalyzerParameterEditor } = require("./AnalyzerParameterEditor.js");
const { AnalyzerScale } = require("./AnalyzerScale.js");
const { uiEditPrecision } = require(
    "../../../Shared/ViewModels/UiEditPrecision.js");

const DEFAULT_SAMPLE_RATE = 48000;
const SPECTRUM_ATTACK = 0.5;
const SPECTRUM_RELEASE = 0.16;

class AnalyzerPresenter extends PresentationObservable
{
    constructor(options)
    {
        super();
        this.options = options || {};
        this.ready = false;
        this.selectedId = 0;
        this.unsubscribers = [];
        this.scale = new AnalyzerScale(
            this.options.frequencyRange,
            this.options.gainRange
        );
        this.sampleRate = Number(this.options.sampleRate) > 0
            ? Number(this.options.sampleRate)
            : DEFAULT_SAMPLE_RATE;
        this.focusedBankId = 0;
        this.targetInstanceId = null;
        this.sourceInstanceId = null;
        this.bankBypass = this.options.bankBypass || null;
        this.curveCalculator = new AnalyzerCurveCalculator(
            this.options.parameters,
            this.bankBypass,
            this.scale,
            this.options.sampleRate
        );
        this.curvePublisher = new AnalyzerCurvePublisher();
        this.spectrum = null;
        this.referenceSpectrum = null;
        this.curves = [];
        this.combinedCurve = null;
        this.spectrumListeners = [];
        this.parameterEditor = new AnalyzerParameterEditor(
            this.options.parameters,
            this.scale,
            () => this.ready,
            () => this.requestRebuild()
        );
        this.parameterEditor.precisionMode =
            this.options.precisionMode || uiEditPrecision;
        this.precisionUnsubscriber = this.parameterEditor.precisionMode.subscribe(
            () => { this.requestRebuild(); });
        this.scope = this.options.scope || null;
        this.scopeUnsubscriber = this.scope && typeof this.scope.subscribe === "function"
            ? this.scope.subscribe(() => { this.requestRebuild(); }) : null;
        this.subscribeParameters();
        this.subscribeStatus();
        this.rebuild();
    }
    
    subscribeParameters()
    {
        let bindings = (this.options.parameters || []).reduce((values, parameter) => {
            return values.concat([
                parameter.frequency,
                parameter.gain,
                parameter.q,
                parameter.enabled
            ]);
        }, []);
        if (this.bankBypass) {
            bindings.push(this.bankBypass);
        }
        bindings.forEach((source) => {
            if (!source) {
                return;
            }
            subscribePresentationBinding(source, () => {
                this.parameterEditor.sourceChanged();
                if (this.ready) {
                    this.requestRebuild();
                }
            }, this.unsubscribers);
        });
    }
    
    subscribeStatus()
    {
        let statusSource = this.options.statusSource;
        if (!statusSource || typeof statusSource.subscribeStatus !== "function") {
            this.hasTargetStatus = false;
            this.ready = true;
            return;
        }
        this.hasTargetStatus = true;
        this.unsubscribers.push(statusSource.subscribeStatus((status) => {
            this.ready = Boolean(status && status.ready);
            if (!this.ready) {
                this.selectedId = 0;
                this.targetInstanceId = null;
                this.sourceInstanceId = null;
                this.spectrum = null;
                this.referenceSpectrum = null;
                this.focusedBankId = null;
            }
            else {
                let nextSourceInstanceId = status.target && status.target.instanceId !== undefined
                    ? String(status.target.instanceId) : null;
                if (nextSourceInstanceId !== this.targetInstanceId) {
                    this.targetInstanceId = nextSourceInstanceId;
                    this.sourceInstanceId = null;
                    this.spectrum = null;
                    this.referenceSpectrum = null;
                }
                this.focusedBankId = status.target
                    ? Number(status.target.bankId) : null;
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
            if (!args || args.length < 3 ||
                    !this.ready && this.hasTargetStatus) {
                return;
            }
            let nextSourceInstanceId = String(args[1]);
            if (this.targetInstanceId !== null &&
                    nextSourceInstanceId !== this.targetInstanceId) {
                return;
            }
            let sourceChanged = this.sourceInstanceId !== null &&
                nextSourceInstanceId !== this.sourceInstanceId;
            this.sourceInstanceId = nextSourceInstanceId;
            let fftSize = Number(args[2]);
            let binCount = Math.floor(fftSize / 2) + 1;
            if (!isFinite(fftSize) || binCount <= 1 ||
                    args.length < 3 + binCount * 2) {
                return;
            }
            this.spectrum = {
                active: true,
                fftSize: fftSize,
                sampleRate: this.sampleRate,
                values: this.smoothSpectrum(
                    sourceChanged ? null : this.spectrum && this.spectrum.values,
                    args.slice(3, 3 + binCount).map(Number))
            };
            let referenceValues = args.slice(
                3 + binCount, 3 + binCount * 2).map(Number);
            this.referenceSpectrum = {
                active: true,
                fftSize: fftSize,
                sampleRate: this.sampleRate,
                values: this.smoothSpectrum(
                    sourceChanged ? null : this.referenceSpectrum &&
                        this.referenceSpectrum.values,
                    referenceValues)
            };
            this.publishSpectrum();
        }));
    }

    smoothSpectrum(previous, current)
    {
        if (!previous || previous.length !== current.length) {
            return current;
        }
        return current.map((value, index) => {
            let previousValue = Number(previous[index]);
            if (!isFinite(previousValue) || !isFinite(value)) {
                return isFinite(value) ? value : 1;
            }
            let coefficient = value < previousValue
                ? SPECTRUM_ATTACK : SPECTRUM_RELEASE;
            return previousValue + (value - previousValue) * coefficient;
        });
    }
    
    publishSpectrum()
    {
        let listeners = this.spectrumListeners.slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](this.spectrum, this.referenceSpectrum);
        }
    }

    subscribeSpectrum(callback, immediate)
    {
        this.spectrumListeners.push(callback);
        if (immediate && this.spectrum) {
            callback(this.spectrum, this.referenceSpectrum);
        }
        return () => {
            this.spectrumListeners = this.spectrumListeners.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    subscribeCurves(callback, immediate)
    {
        return this.curvePublisher.subscribe(
            callback,
            immediate,
            this.curves,
            this.combinedCurve
        );
    }
    
    publishCurves()
    {
        this.curvePublisher.publish(this.curves, this.combinedCurve);
    }
    
    read(source, fallback)
    {
        return presentationBindingValue(source, fallback);
    }
    
    rebuild()
    {
        if (this.ready) {
            this.calculateCurves();
        }
        else {
            this.curves = [];
            this.combinedCurve = null;
        }
        let presentation = new AnalyzerPresentation();
        presentation.mode = this.options.mode || "equalizer";
        presentation.enabled = this.ready;
        presentation.scopeActive = Boolean(this.scope && this.scope.isGroup());
        presentation.scopeColor = presentation.scopeActive ? this.scope.color : null;
        presentation.frequencyMinimum = this.scale.frequencyMinimum;
        presentation.frequencyMaximum = this.scale.frequencyMaximum;
        presentation.gainMinimum = this.scale.gainMinimum;
        presentation.gainMaximum = this.scale.gainMaximum;
        presentation.precisionRough = this.parameterEditor.precisionMode.isRough();
        presentation.roughGrid = presentation.precisionRough
            ? createRoughGrid(this.scale) : null;
        presentation.spectrum = this.spectrum;
        presentation.referenceSpectrum = this.referenceSpectrum;
        presentation.combinedCurve = this.combinedCurve;
        presentation.curves = this.curves;
        if (this.ready) {
            (this.options.parameters || []).forEach((parameter, index) => {
                let frequencyMinimum = this.scale.clampBindingValue(
                    parameter.frequency, this.scale.frequencyMinimum);
                let frequencyMaximum = this.scale.clampBindingValue(
                    parameter.frequency, this.scale.frequencyMaximum);
                let gainMinimum = this.scale.clampBindingValue(
                    parameter.gain, this.scale.gainMinimum);
                let gainMaximum = this.scale.clampBindingValue(
                    parameter.gain, this.scale.gainMaximum);
                if (!parameter.gain) {
                    return;
                }
                let hasFrequency = Boolean(parameter.frequency);
                let preview = this.parameterEditor.preview[index + 1];
                let frequencyValue = hasFrequency && preview &&
                    preview.frequency !== undefined
                    ? preview.frequency : this.read(parameter.frequency, 1000);
                let gainValue = preview && preview.gain !== undefined
                    ? preview.gain : this.read(parameter.gain, 0);
                presentation.handles.push({
                    id: index + 1,
                    frequency: hasFrequency
                        ? this.scale.frequencyToX(frequencyValue) : 0,
                    gain: this.scale.gainToY(gainValue),
                    enabled: this.read(parameter.enabled, true),
                    selected: index + 1 === this.selectedId,
                    xMinimum: hasFrequency ? this.scale.frequencyToX(frequencyMinimum) : 0,
                    xMaximum: hasFrequency ? this.scale.frequencyToX(frequencyMaximum) : 0,
                    yMinimum: this.scale.gainToY(gainMaximum),
                    yMaximum: this.scale.gainToY(gainMinimum),
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

    calculateCurves()
    {
        const result = this.curveCalculator.calculate(this.parameterEditor.preview);
        this.curves = result.curves;
        this.combinedCurve = result.combinedCurve;
        this.publishCurves();
    }

    previewMoved(id, x, y)
    {
        this.parameterEditor.previewMoved(id, x, y);
    }

    beginPreviewGesture()
    {
        this.parameterEditor.beginGesture();
    }

    endPreviewGesture()
    {
        this.parameterEditor.endGesture();
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
        this.parameterEditor.move(id, x, y, transactionId);
    }

    filterQChanged(id, delta, transactionId)
    {
        this.parameterEditor.changeQ(id, delta, transactionId);
    }

    previewQChanged(id, delta)
    {
        this.parameterEditor.previewQ(id, delta);
    }

    commitQ(id, transactionId)
    {
        this.parameterEditor.commitQ(id, transactionId);
    }

    resetFilter(id, callback)
    {
        this.parameterEditor.reset(id, callback);
    }

    commitPreview(id, x, y, transactionId, callback)
    {
        this.parameterEditor.commit(id, x, y, transactionId, callback);
    }
    
    destroy()
    {
        if (this.scopeUnsubscriber) this.scopeUnsubscriber();
        if (this.precisionUnsubscriber) this.precisionUnsubscriber();
        this.unsubscribers.forEach((unsubscribe) => {
            unsubscribe();
        });
        this.unsubscribers = [];
        this.spectrumListeners = [];
        this.curvePublisher.clear();
        this.sourceInstanceId = null;
        this.parameterEditor.clear();
        super.destroy();
    }

}

function createRoughGrid(scale)
{
    const frequencies = [];
    const frequencySteps = Math.max(1, Math.floor(
        Math.log10(scale.frequencyMaximum / scale.frequencyMinimum) * 10));
    for (let index = 0; index <= frequencySteps; index += 1) {
        const frequency = scale.frequencyMinimum * Math.pow(10, index / 10);
        frequencies.push(scale.frequencyToX(frequency));
    }
    if (frequencies[frequencies.length - 1] < 1 - 0.0000001)
    {
        frequencies.push(1);
    }

    const gains = [];
    for (let gain = Math.ceil(scale.gainMinimum / 3) * 3;
        gain <= scale.gainMaximum; gain += 3) {
        gains.push(scale.gainToY(gain));
    }
    return { frequencies: frequencies, gains: gains };
}


module.exports = {
    AnalyzerPresenter: AnalyzerPresenter
};
