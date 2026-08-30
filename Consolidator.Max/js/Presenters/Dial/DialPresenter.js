const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { clampPresentationValue } = require("../Core/Normalization.js");
const { normalizePresentationValue } = require("../Core/Normalization.js");
const { denormalizePresentationValue } = require("../Core/Normalization.js");
const { presentationBindingSource } = require("../Core/PresentationBinding.js");
const { presentationBindingValue } = require("../Core/PresentationBinding.js");
const { presentationBindingWrite } = require("../Core/PresentationBinding.js");
const { subscribePresentationBinding } = require("../Core/PresentationBinding.js");
const { DialPresentation } = require("./DialPresentation.js");

class DialPresenter extends PresentationObservable
{
    constructor(options)
    {
        super();
        this.options = options || {};
        this.maximumRingCount = this.options.maximumRingCount === undefined
            ? 3 : Math.max(1, Math.floor(this.options.maximumRingCount));
        this.state = {
            activeIndex: 0,
            displayIndex: 0
        };
        this.ringMappings = [];
        this.unsubscribers = [];
        this.eventListeners = {};
    
        this.subscribeSources();
        this.rebuild();
    }
    
    on(eventName, callback)
    {
        if (typeof callback !== "function" || this.destroyed) {
            return () => {};
        }
        if (!this.eventListeners[eventName]) {
            this.eventListeners[eventName] = [];
        }
        this.eventListeners[eventName].push(callback);
        return () => {
            this.off(eventName, callback);
        };
    }
    
    off(eventName, callback)
    {
        let listeners = this.eventListeners[eventName] || [];
        this.eventListeners[eventName] = listeners.filter((listener) => {
            return listener !== callback;
        });
    }
    
    emit(eventName, payload)
    {
        let listeners = (this.eventListeners[eventName] || []).slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](payload);
        }
    }
    
    subscribeSources()
    {
        const subscribedSources = [];
        const subscribe = (source) => {
            const bindingSource = presentationBindingSource(source);
            if (!bindingSource || subscribedSources.indexOf(bindingSource) >= 0) {
                return;
            }
            subscribedSources.push(bindingSource);
            subscribePresentationBinding(source, () => {
                this.requestRebuild();
            }, this.unsubscribers);
        };
    
        let rings = this.options.rings || [];
        for (let index = 0; index < Math.min(rings.length, this.maximumRingCount); index += 1) {
            let ring = rings[index] || {};
            subscribe(ring.value);
            subscribe(ring.minimum);
            subscribe(ring.maximum);
            subscribe(ring.physicalMinimum);
            subscribe(ring.physicalMaximum);
            subscribe(ring.defaultValue);
            subscribe(ring.physicalStep);
            subscribe(ring.visualization);
            subscribe(ring.color);
        }
        subscribe(this.options.enabled);
        subscribe(this.options.loading);
        subscribe(this.options.active);
        subscribe(this.options.scope);
    }
    
    read(source, fallback)
    {
        return presentationBindingValue(source, fallback);
    }
    
    readNumber(source, fallback)
    {
        let value = this.read(source, fallback);
        return typeof value === "number" && isFinite(value) ? value : fallback;
    }
    
    buildRing(configuration)
    {
        configuration = configuration || {};
        let valueSource = configuration.value;
        let valueModel = valueSource && valueSource.value !== undefined
            ? valueSource : {};
        let physicalMinimum = this.readNumber(
            configuration.physicalMinimum,
            this.readNumber(valueModel.physicalMinimum, 0)
        );
        let physicalMaximum = this.readNumber(
            configuration.physicalMaximum,
            this.readNumber(valueModel.physicalMaximum, 1)
        );
        let display = configuration.display || valueModel.display || {};
        let mapping = configuration.mapping || {};
        let logarithmic = mapping.type === "logarithmic";
        let minimum = this.readNumber(
            configuration.minimum,
            this.readNumber(valueModel.minimum, physicalMinimum)
        );
        let maximum = this.readNumber(
            configuration.maximum,
            this.readNumber(valueModel.maximum, physicalMaximum)
        );
        let value = this.readNumber(valueSource, physicalMinimum);
        let hasDefaultValue = configuration.defaultValue !== undefined
            || valueModel.defaultValue !== undefined;
        let defaultValue = hasDefaultValue
            ? this.readNumber(
                configuration.defaultValue,
                this.readNumber(valueModel.defaultValue, value)
            )
            : null;
        let visualization = this.read(
            configuration.visualization,
            valueModel.visualization || null
        );
    
        let ring = {
            value: normalizePresentationValue(
                value, physicalMinimum, physicalMaximum, logarithmic
            ),
            minimum: normalizePresentationValue(
                minimum, physicalMinimum, physicalMaximum, logarithmic
            ),
            maximum: normalizePresentationValue(
                maximum, physicalMinimum, physicalMaximum, logarithmic
            ),
            defaultValue: defaultValue === null ? null : normalizePresentationValue(
                defaultValue, physicalMinimum, physicalMaximum, logarithmic
            ),
            display: {
                value: this.formatDisplayValue(value, display)
            },
            visualization: this.buildVisualization(visualization),
            color: this.read(configuration.color, valueModel.color || null)
        };
        this.ringMappings.push({
            physicalMinimum: physicalMinimum,
            physicalMaximum: physicalMaximum,
            physicalStep: this.readNumber(
                configuration.physicalStep,
                this.readNumber(valueModel.physicalStep, 0)
            ),
            logarithmic: logarithmic
        });
        return ring;
    }
    
    formatDisplayValue(value, display)
    {
        let scale = display.scale === undefined ? 1 : Number(display.scale);
        if (!isFinite(scale)) scale = 1;
        value *= scale;
        let decimals = display.decimals === undefined ? 2
            : Math.max(0, Math.floor(Number(display.decimals)));
        let suffix = display.suffix === undefined ? "" : String(display.suffix);
        return Number(value).toFixed(decimals) + suffix;
    }
    
    buildVisualization(source)
    {
        let visualization = source || {};
        let type = visualization.type || "none";
        if (type === "none") return null;
        let range = visualization.range || {};
        let minimum = this.readNumber(range.minimum, 0);
        let maximum = this.readNumber(range.maximum, 1);
        if (type === "level") {
            return {
                type: "level",
                peak: normalizePresentationValue(
                    this.readNumber(visualization.peak, minimum), minimum, maximum
                ),
                smoothed: normalizePresentationValue(
                    this.readNumber(visualization.smoothed, minimum), minimum, maximum
                )
            };
        }
        if (type === "reduction" || type === "saturation") {
            return {
                type: type,
                value: normalizePresentationValue(
                    this.readNumber(visualization.value, minimum), minimum, maximum
                )
            };
        }
        if (type === "relative") {
            return {
                type: "relative",
                value: this.clampRelative(
                    this.readNumber(visualization.value, 0)
                )
            };
        }
        return null;
    }
    
    clampRelative(value)
    {
        return Math.max(-1, Math.min(1, value));
    }
    
    rebuild()
    {
        let presentation = new DialPresentation();
        presentation.enabled = this.read(this.options.enabled, true);
        presentation.loading = this.read(this.options.loading, false);
        presentation.active = this.read(this.options.active, true);
        let scope = this.options.scope;
        presentation.groupScope = Boolean(scope &&
            typeof scope.isGroup === "function" && scope.isGroup());
        presentation.scopeActive = presentation.groupScope;
        presentation.scopeColor = presentation.groupScope ? scope.color : null;
    
        let rings = this.options.rings || [];
        this.ringMappings = [];
        for (let index = 0; index < Math.min(rings.length, this.maximumRingCount); index += 1) {
            presentation.rings.push(this.buildRing(rings[index]));
        }
        presentation.activeIndex = this.normalizeIndex(
            this.state.activeIndex, presentation.rings.length
        );
        presentation.displayIndex = this.normalizeIndex(
            this.state.displayIndex, presentation.rings.length
        );
        this.publish(presentation);
    }
    
    normalizeIndex(value, count)
    {
        let index = Number(this.read(value, 0));
        if (!isFinite(index) || count === 0) return 0;
        return Math.max(0, Math.min(count - 1, Math.floor(index)));
    }
    
    setValue(ringIndex, normalizedValue, transactionId)
    {
        let configuration = (this.options.rings || [])[ringIndex];
        let ring = this.presentation && this.presentation.rings[ringIndex];
        let mapping = this.ringMappings[ringIndex];
        if (!configuration || !ring || !mapping || !configuration.value) {
            return;
        }
    
        let clampedValue = clampPresentationValue(
            normalizedValue, ring.minimum, ring.maximum
        );
        let physicalValue = denormalizePresentationValue(
            clampedValue,
            mapping.physicalMinimum,
            mapping.physicalMaximum,
            mapping.logarithmic
        );
        let physicalStep = Number(mapping.physicalStep);
        if (isFinite(physicalStep) && physicalStep > 0) {
            physicalValue = mapping.physicalMinimum + Math.round(
                (physicalValue - mapping.physicalMinimum) / physicalStep
            ) * physicalStep;
        }
        let physicalMinimum = denormalizePresentationValue(
            ring.minimum,
            mapping.physicalMinimum,
            mapping.physicalMaximum,
            mapping.logarithmic
        );
        let physicalMaximum = denormalizePresentationValue(
            ring.maximum,
            mapping.physicalMinimum,
            mapping.physicalMaximum,
            mapping.logarithmic
        );
        physicalValue = clampPresentationValue(
            physicalValue, physicalMinimum, physicalMaximum
        );
        presentationBindingWrite(
            configuration.value, physicalValue, transactionId);
    }
    
    resetValue(ringIndex, transactionId)
    {
        let configuration = (this.options.rings || [])[ringIndex];
        let source = configuration && presentationBindingSource(configuration.value);
        if (source && typeof source.reset === "function") {
            source.reset(transactionId);
        }
    }
    
    setActive(value)
    {
        presentationBindingWrite(this.options.active, value);
    }
    
    setActiveIndex(index)
    {
        let count = this.presentation ? this.presentation.rings.length : 0;
        let nextIndex = this.normalizeIndex(index, count);
        this.state.activeIndex = nextIndex;
        this.rebuild();
    }
    
    beginGesture(ringIndex)
    {
        this.emit("gestureBegan", { index: ringIndex });
    }
    
    endGesture(ringIndex)
    {
        this.emit("gestureEnded", { index: ringIndex });
    }
    
    destroy()
    {
        if (this.destroyed) return;
        for (let index = 0; index < this.unsubscribers.length; index += 1) {
            this.unsubscribers[index]();
        }
        this.unsubscribers = [];
        this.eventListeners = {};
        super.destroy();
    }
}


module.exports = {
    DialPresenter: DialPresenter
};
