const { PresentationObservable } = require("../../Presenters/PresentationObservable.js");
const { presentationBindingSource } = require("../../Presenters/PresentationBinding.js");
const { presentationBindingValue } = require("../../Presenters/PresentationBinding.js");
const { presentationBindingWrite } = require("../../Presenters/PresentationBinding.js");
const { subscribePresentationBinding } = require("../../Presenters/PresentationBinding.js");
const { DialPresentation } = require("./DialPresentation.js");
const { DialRingBuilder } = require("./DialRingBuilder.js");
const { resetDialValue, writeDialValue } = require("./DialValueEditor.js");

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
        this.ringBuilder = new DialRingBuilder(this.options.scope);
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
    
        const result = this.ringBuilder.build(
            this.options.rings,
            this.maximumRingCount
        );
        presentation.rings = result.rings;
        this.ringMappings = result.mappings;
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
        writeDialValue(
            configuration,
            ring,
            mapping,
            normalizedValue,
            transactionId
        );
    }
    
    resetValue(ringIndex, transactionId)
    {
        let configuration = (this.options.rings || [])[ringIndex];
        resetDialValue(configuration, transactionId);
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
