const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { presentationBindingSource } = require("../Core/PresentationBinding.js");
const { presentationBindingValue } = require("../Core/PresentationBinding.js");
const { presentationBindingWrite } = require("../Core/PresentationBinding.js");
const { subscribePresentationBinding } = require("../Core/PresentationBinding.js");
const { AnalyzerPresentation } = require("./AnalyzerPresentation.js");
const { BiquadCalculator } = require("./BiquadCalculator.js");

const DEFAULT_SAMPLE_RATE = 48000;

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
        this.sampleRate = Number(this.options.sampleRate) > 0
            ? Number(this.options.sampleRate) : DEFAULT_SAMPLE_RATE;
        this.sampleRates = {};
        this.equalizerStates = {};
        this.filterCatalogs = {};
        this.equalizerState = null;
        this.otherBanksDecibels = null;
        this.focusedBankId = 0;
        this.sourceInstanceId = null;
        this.curvePointCount = 256;
        this.curveFrequencies = this.createCurveFrequencies();
        this.bankBypass = this.options.bankBypass || null;
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
        this.curvePreview = {};
        this.previewGestureActive = false;
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
                if (!this.previewGestureActive) {
                    this.curvePreview = {};
                }
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
                this.sourceInstanceId = null;
                this.focusedBankId = null;
                this.equalizerState = null;
            }
            else {
                this.focusedBankId = status.target
                    ? Number(status.target.bankId) : null;
                const configuredSampleRate = this.sourceInstanceId === null
                    ? null : this.sampleRates[String(this.sourceInstanceId)];
                this.sampleRate = configuredSampleRate || DEFAULT_SAMPLE_RATE;
                this.equalizerState = this.sourceInstanceId === null
                    ? null : this.equalizerStates[String(this.sourceInstanceId)] || null;
            }
            this.otherBanksDecibels = null;
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
                    (this.sourceInstanceId !== null &&
                        String(args[1]) !== String(this.sourceInstanceId))) {
                return;
            }
            this.sourceInstanceId = String(args[1]);
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

    connectConfiguration(protocol)
    {
        if (!protocol || typeof protocol.on !== "function") {
            return;
        }
        this.unsubscribers.push(protocol.on("analyzer_configuration", (args) => {
            if (!args || Number(args[0]) !== 1 || args.length < 3) {
                return;
            }
            const sampleRate = Number(args[2]);
            if (!isFinite(sampleRate) || sampleRate <= 0) {
                return;
            }
            const sourceInstanceId = String(args[1]);
            this.sampleRates[sourceInstanceId] = sampleRate;
            this.sourceInstanceId = sourceInstanceId;
            if (sampleRate === this.sampleRate) {
                return;
            }
            this.sampleRate = sampleRate;
            this.otherBanksDecibels = null;
            this.requestRebuild();
        }));
        this.unsubscribers.push(protocol.on("analyzer_equalizer_state", (args) => {
            const state = this.decodeEqualizerState(args);
            if (!state) {
                return;
            }
            this.sourceInstanceId = state.sourceInstanceId;
            this.equalizerStates[state.sourceInstanceId] = state;
            const preserveOtherBanks = this.sameOtherBanksState(
                this.equalizerState,
                state);
            this.equalizerState = state;
            if (!preserveOtherBanks) {
                this.otherBanksDecibels = null;
            }
            this.requestRebuild();
        }));
        this.unsubscribers.push(protocol.on("filter_catalog", (args) => {
            const catalog = this.decodeFilterCatalog(args);
            if (!catalog) {
                return;
            }
            this.sourceInstanceId = catalog.sourceInstanceId;
            this.filterCatalogs[
                String(catalog.sourceInstanceId) + ":" + catalog.context] =
                catalog.filters;
            this.otherBanksDecibels = null;
            this.requestRebuild();
        }));
    }

    sameOtherBanksState(first, second)
    {
        if (!first || !second ||
                first.sourceInstanceId !== second.sourceInstanceId ||
                first.banks.length !== second.banks.length) {
            return false;
        }
        for (let bankIndex = 0; bankIndex < first.banks.length;
                bankIndex += 1) {
            if (bankIndex === this.focusedBankId) {
                continue;
            }
            let firstBank = first.banks[bankIndex];
            let secondBank = second.banks[bankIndex];
            if (firstBank.active !== secondBank.active ||
                    firstBank.filters.length !== secondBank.filters.length) {
                return false;
            }
            for (let filterIndex = 0; filterIndex < firstBank.filters.length;
                    filterIndex += 1) {
                let firstFilter = firstBank.filters[filterIndex];
                let secondFilter = secondBank.filters[filterIndex];
                if (firstFilter.active !== secondFilter.active ||
                        firstFilter.type !== secondFilter.type ||
                        firstFilter.fixedQ !== secondFilter.fixedQ ||
                        firstFilter.frequency !== secondFilter.frequency ||
                        firstFilter.q !== secondFilter.q ||
                        firstFilter.gain !== secondFilter.gain) {
                    return false;
                }
            }
        }
        return true;
    }

    decodeEqualizerState(args)
    {
        if (!args || Number(args[0]) !== 1 || Number(args[1]) !== 2 ||
                args.length < 5) {
            return null;
        }
        const bankCount = Math.floor(Number(args[3]));
        const expectedLengthMinimum = 5 + bankCount * 2;
        if (!isFinite(bankCount) || bankCount < 1 || bankCount > 32 ||
                args.length < expectedLengthMinimum) {
            return null;
        }
        let position = 5;
        let banks = [];
        for (let bankIndex = 0; bankIndex < bankCount; bankIndex += 1) {
            let bank = { active: Number(args[position]) !== 0, filters: [] };
            position += 1;
            let filterCount = Math.floor(Number(args[position]));
            position += 1;
            if (!isFinite(filterCount) || filterCount < 1 || filterCount > 32) {
                return null;
            }
            for (let filterIndex = 0; filterIndex < filterCount; filterIndex += 1) {
                let filter = {
                    active: Number(args[position]) !== 0,
                    type: String(args[position + 1]),
                    fixedQ: Number(args[position + 2]),
                    parameters: {}
                };
                let parameterCount = Math.floor(Number(args[position + 3]));
                position += 4;
                if (!isFinite(parameterCount) || parameterCount < 1 || parameterCount > 3 ||
                        position + parameterCount * 2 > args.length) {
                    return null;
                }
                for (let parameterIndex = 0; parameterIndex < parameterCount; parameterIndex += 1) {
                    let name = String(args[position]);
                    let value = Number(args[position + 1]);
                    if (["frequency", "q", "gain"].indexOf(name) < 0 ||
                            filter.parameters[name] !== undefined || !isFinite(value)) {
                        return null;
                    }
                    filter.parameters[name] = value;
                    filter[name] = value;
                    position += 2;
                }
                if (filter.parameters.gain === undefined || !isFinite(filter.fixedQ) || filter.fixedQ <= 0 ||
                        (filter.type !== "gain" && filter.parameters.frequency === undefined) ||
                        (filter.type === "bell" && filter.parameters.q === undefined) ||
                        (filter.type === "gain" && Object.keys(filter.parameters).length !== 1) ||
                        (filter.type === "bell" && Object.keys(filter.parameters).length !== 3) ||
                        (filter.type !== "bell" && filter.type !== "gain" && Object.keys(filter.parameters).length !== 2) ||
                        (filter.type !== "gain" && filter.parameters.q !== undefined && filter.type !== "bell") ||
                        (["bell", "tilt", "low_shelf", "high_shelf", "gain"].indexOf(filter.type) < 0)) {
                    return null;
                }
                bank.filters.push(filter);
            }
            banks.push(bank);
        }
        if (position !== args.length) return null;
        return {
            sourceInstanceId: String(args[2]),
            active: Number(args[4]) !== 0,
            banks: banks
        };
    }

    decodeFilterCatalog(args)
    {
        if (!args || Number(args[0]) !== 1 || args.length < 4) return null;
        let filterCount = Math.floor(Number(args[3]));
        if (!isFinite(filterCount) || filterCount < 1 || filterCount > 32) return null;
        let position = 4;
        let filters = [];
        for (let filterIndex = 0; filterIndex < filterCount; filterIndex += 1) {
            if (position + 4 > args.length) return null;
            let id = Math.floor(Number(args[position]));
            let type = String(args[position + 1]);
            let fixedQ = Number(args[position + 2]);
            let parameterCount = Math.floor(Number(args[position + 3]));
            position += 4;
            if (id !== filterIndex + 1 || !isFinite(parameterCount) ||
                    parameterCount < 1 || parameterCount > 3 || !isFinite(fixedQ) || fixedQ <= 0 ||
                    ["bell", "tilt", "low_shelf", "high_shelf", "gain"].indexOf(type) < 0) return null;
            let parameters = {};
            for (let index = 0; index < parameterCount; index += 1) {
                if (position + 4 > args.length) return null;
                let name = String(args[position]);
                let minimum = Number(args[position + 1]);
                let maximum = Number(args[position + 2]);
                let defaultValue = Number(args[position + 3]);
                position += 4;
                if (["frequency", "q", "gain"].indexOf(name) < 0 ||
                        parameters[name] || !isFinite(minimum) || !isFinite(maximum) ||
                        !isFinite(defaultValue) || minimum > maximum ||
                        defaultValue < minimum || defaultValue > maximum) return null;
                parameters[name] = {
                    minimum: minimum, maximum: maximum, defaultValue: defaultValue
                };
            }
            if (parameters.gain === undefined ||
                    (type === "gain" && Object.keys(parameters).length !== 1) ||
                    (type === "bell" && Object.keys(parameters).length !== 3) ||
                    (type !== "bell" && type !== "gain" && Object.keys(parameters).length !== 2) ||
                    (type !== "bell" && parameters.q !== undefined)) return null;
            filters.push({ type: type, fixedQ: fixedQ, parameters: parameters });
        }
        if (position !== args.length) return null;
        return { sourceInstanceId: String(args[1]), context: String(args[2]), filters: filters };
    }
    
    publishSpectrum()
    {
        let listeners = this.spectrumListeners.slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](this.spectrum, this.referenceSpectrum);
        }
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
        if (this.ready) {
            this.calculateCurves();
        }
        else {
            this.curves = [];
            this.combinedCurve = null;
            this.allBanksCurve = null;
        }
        let presentation = new AnalyzerPresentation();
        presentation.mode = this.options.mode || "equalizer";
        presentation.enabled = this.ready;
        presentation.scopeActive = Boolean(this.scope && this.scope.isGroup());
        presentation.scopeColor = presentation.scopeActive ? this.scope.color : null;
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
                if (!parameter.gain) {
                    return;
                }
                let hasFrequency = Boolean(parameter.frequency);
                presentation.handles.push({
                    id: index + 1,
                    frequency: hasFrequency ? this.frequencyToX(
                        this.read(parameter.frequency, 1000)) : 0,
                    gain: this.gainToY(this.read(parameter.gain, 0)),
                    enabled: this.read(parameter.enabled, true),
                    selected: index + 1 === this.selectedId,
                    xMinimum: hasFrequency ? this.frequencyToX(frequencyMinimum) : 0,
                    xMaximum: hasFrequency ? this.frequencyToX(frequencyMaximum) : 0,
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

    calculateCurves()
    {
        let parameters = this.options.parameters || [];
        let bankActive = !this.read(this.bankBypass, false);
        let responses = parameters.map((parameter, index) => {
            let preview = this.curvePreview[index + 1];
            let enabled = Boolean(this.read(parameter.enabled, true));
            return this.calculateFilterCurve(
                index + 1, parameter, enabled, preview);
        });
        let combinedDecibels = new Array(this.curvePointCount).fill(0);
        for (let point = 0; point < this.curvePointCount; point += 1) {
            if (bankActive) {
                responses.forEach((response) => {
                    if (response.active) {
                        combinedDecibels[point] += response.decibels[point];
                    }
                });
            }
        }
        let combined = combinedDecibels.map((decibels) => {
            return this.toNormalizedDecibels(decibels);
        });
        this.curves = responses.map((response) => {
            return {
                id: response.id,
                active: response.active,
                values: response.values
            };
        });
        this.combinedCurve = { active: bankActive, values: combined };
        this.allBanksCurve = this.calculateAllBanksCurve(
            combinedDecibels,
            bankActive);
        this.publishCurves();
    }

    calculateAllBanksCurve(focusedDecibels, focusedBankActive)
    {
        if ((this.options.mode || "equalizer") !== "equalizer") {
            return null;
        }
        if (!this.equalizerState) {
            return {
                active: false,
                values: []
            };
        }
        if (!this.otherBanksDecibels) {
            this.otherBanksDecibels = new Array(this.curvePointCount).fill(0);
            this.equalizerState.banks.forEach((bank, bankIndex) => {
                if (!bank.active || bankIndex === this.focusedBankId) {
                    return;
                }
                bank.filters.forEach((filter, filterIndex) => {
                    if (!filter.active) {
                        return;
                    }
                    let response = this.calculateFilterCurve(
                        filterIndex + 1,
                        filter,
                        true,
                        null);
                    for (let point = 0; point < this.curvePointCount; point += 1) {
                        this.otherBanksDecibels[point] += response.decibels[point];
                    }
                });
            });
        }
        let decibels = this.otherBanksDecibels.slice(0);
        if (focusedBankActive) {
            for (let point = 0; point < this.curvePointCount; point += 1) {
                decibels[point] += focusedDecibels[point];
            }
        }
        return {
            active: this.equalizerState.active,
            values: decibels.map((value) => {
                return this.toNormalizedDecibels(value);
            })
        };
    }

    calculateFilterCurve(id, parameter, enabled, preview)
    {
        let definition = this.getFilterDefinition(id, parameter);
        const filterFrequency = preview && preview.frequency !== undefined
            ? Number(preview.frequency)
            : Number(this.read(parameter.frequency, 1000));
        const q = preview && preview.q !== undefined
            ? Number(preview.q)
            : parameter.q
                ? Number(this.read(parameter.q, definition.fixedQ || 1))
                : Number(definition.fixedQ || 1);
        const gain = preview && preview.gain !== undefined
            ? Number(preview.gain)
            : Number(this.read(parameter.gain, 0));
        const type = definition.type;
        const valid = enabled && isFinite(gain) &&
            (type === "gain" || (isFinite(filterFrequency) &&
                isFinite(q) && filterFrequency > 0 && q > 0));
        const coefficients = valid
            ? BiquadCalculator.calculate(
                type,
                filterFrequency, q, gain, this.sampleRate)
            : null;
        const decibels = this.curveFrequencies.map((frequency) => {
            return coefficients
                ? BiquadCalculator.decibelsAt(
                    coefficients, frequency, this.sampleRate)
                : 0;
        });
        return {
            id: id,
            active: enabled,
            decibels: decibels,
            values: decibels.map((value) => {
                return this.toNormalizedDecibels(value);
            })
        };
    }

    getFilterDefinition(id, parameter)
    {
        let catalog = this.filterCatalogs[
            String(this.sourceInstanceId) + ":" + this.options.context];
        let definition = catalog && catalog[Number(id) - 1];
        if (definition) {
            return definition;
        }
        return parameter.definition || {
            type: parameter.type || "bell",
            fixedQ: 1,
            parameters: {}
        };
    }

    createCurveFrequencies()
    {
        let frequencies = [];
        for (let point = 0; point < this.curvePointCount; point += 1) {
            let normalized = point / (this.curvePointCount - 1);
            frequencies.push(this.frequencyMinimum * Math.pow(
                this.frequencyMaximum / this.frequencyMinimum,
                normalized));
        }
        return frequencies;
    }

    previewMoved(id, x, y)
    {
        let parameter = (this.options.parameters || [])[Number(id) - 1];
        if (!parameter || !parameter.gain || !this.ready) {
            return;
        }
        let gain = this.clampParameterValue(
            parameter.gain, this.yToGain(y));
        let preview = { gain: gain };
        if (parameter.frequency) {
            preview.frequency = this.clampParameterValue(
                parameter.frequency, this.xToFrequency(x));
        }
        this.curvePreview[Number(id)] = preview;
        this.requestRebuild();
    }

    beginPreviewGesture()
    {
        this.curvePreview = {};
        this.previewGestureActive = true;
    }

    endPreviewGesture()
    {
        if (!this.previewGestureActive &&
                Object.keys(this.curvePreview).length === 0) {
            return;
        }
        this.previewGestureActive = false;
        this.curvePreview = {};
        this.requestRebuild();
    }

    toNormalizedDecibels(decibels)
    {
        return Math.max(0, Math.min(1, 1 - (decibels + 24) / 48));
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
        if (!parameter || !parameter.gain || !this.ready) {
            return;
        }
        let gain = this.yToGain(y);
        gain = this.clampParameterValue(parameter.gain, gain);
        let frequency = parameter.frequency
            ? this.clampParameterValue(parameter.frequency, this.xToFrequency(x))
            : null;
        this.curvePreview[Number(id)] = { frequency: frequency, gain: gain };
        this.requestRebuild();
        if (typeof parameter.setPosition === "function") {
            parameter.setPosition(frequency, gain, transactionId);
            return;
        }
        if (parameter.frequency && frequency !== null) {
            presentationBindingWrite(parameter.frequency, frequency, transactionId);
        }
        presentationBindingWrite(parameter.gain, gain, transactionId);
    }

    resetFilter(id, callback)
    {
        let parameter = (this.options.parameters || [])[Number(id) - 1];
        if (!parameter || !this.ready || typeof parameter.reset !== "function") {
            if (callback) callback({ status: "accepted", error: null });
            return;
        }
        parameter.reset(callback);
        this.curvePreview[Number(id)] = null;
        this.requestRebuild();
    }

    commitPreview(id, transactionId, callback)
    {
        let preview = this.curvePreview[Number(id)];
        let parameter = (this.options.parameters || [])[Number(id) - 1];
        if (!preview || preview.gain === undefined || !parameter ||
                !parameter.gain || (parameter.frequency &&
                    preview.frequency === undefined)) {
            if (callback) callback({ status: "accepted", error: null });
            return;
        }
        if (typeof parameter.setPosition === "function") {
            parameter.setPosition(
                preview.frequency,
                preview.gain,
                transactionId,
                callback);
            return;
        }
        if (parameter.frequency) {
            presentationBindingWrite(
                parameter.frequency, preview.frequency, transactionId);
        }
        presentationBindingWrite(
            parameter.gain, preview.gain, transactionId);
        if (callback) callback({ status: "accepted", error: null });
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
        this.curvePreview[Number(id)] = { q: next };
        this.requestRebuild();
        presentationBindingWrite(parameter.q, next);
    }
    
    destroy()
    {
        if (this.scopeUnsubscriber) this.scopeUnsubscriber();
        this.unsubscribers.forEach((unsubscribe) => {
            unsubscribe();
        });
        this.unsubscribers = [];
        this.spectrumListeners = [];
        this.curveListeners = [];
        this.sampleRates = {};
        this.equalizerStates = {};
        this.filterCatalogs = {};
        this.equalizerState = null;
        this.otherBanksDecibels = null;
        this.sourceInstanceId = null;
        this.previewGestureActive = false;
        this.curvePreview = {};
        super.destroy();
    }

}


module.exports = {
    AnalyzerPresenter: AnalyzerPresenter
};
