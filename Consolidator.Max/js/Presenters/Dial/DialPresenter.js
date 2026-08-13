include("../Core/PresentationObservable.js");
include("../Core/Normalization.js");
include("../Core/PresentationBinding.js");
include("DialPresentation.js");

function DialPresenter(options) {
    PresentationObservable.call(this);
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

DialPresenter.prototype = Object.create(PresentationObservable.prototype);
DialPresenter.prototype.constructor = DialPresenter;

DialPresenter.prototype.on = function (eventName, callback) {
    if (typeof callback !== "function" || this.destroyed) {
        return function () {};
    }
    if (!this.eventListeners[eventName]) {
        this.eventListeners[eventName] = [];
    }
    this.eventListeners[eventName].push(callback);
    var self = this;
    return function () {
        self.off(eventName, callback);
    };
};

DialPresenter.prototype.off = function (eventName, callback) {
    var listeners = this.eventListeners[eventName] || [];
    this.eventListeners[eventName] = listeners.filter(function (listener) {
        return listener !== callback;
    });
};

DialPresenter.prototype.emit = function (eventName, payload) {
    var listeners = (this.eventListeners[eventName] || []).slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](payload);
    }
};

DialPresenter.prototype.subscribeSources = function () {
    var self = this;
    function subscribe(source) {
        subscribePresentationBinding(source, function () {
            self.rebuild();
        }, self.unsubscribers);
    }

    var rings = this.options.rings || [];
    for (var index = 0; index < Math.min(rings.length, this.maximumRingCount); index += 1) {
        var ring = rings[index] || {};
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
    subscribe(this.options.active);
};

DialPresenter.prototype.read = function (source, fallback) {
    return presentationBindingValue(source, fallback);
};

DialPresenter.prototype.readNumber = function (source, fallback) {
    var value = this.read(source, fallback);
    return typeof value === "number" && isFinite(value) ? value : fallback;
};

DialPresenter.prototype.buildRing = function (configuration) {
    configuration = configuration || {};
    var valueSource = configuration.value;
    var valueModel = valueSource && valueSource.value !== undefined
        ? valueSource : {};
    var physicalMinimum = this.readNumber(
        configuration.physicalMinimum,
        this.readNumber(valueModel.physicalMinimum, 0)
    );
    var physicalMaximum = this.readNumber(
        configuration.physicalMaximum,
        this.readNumber(valueModel.physicalMaximum, 1)
    );
    var display = configuration.display || valueModel.display || {};
    var mapping = configuration.mapping || {};
    var logarithmic = mapping.type === "logarithmic";
    var minimum = this.readNumber(
        configuration.minimum,
        this.readNumber(valueModel.minimum, physicalMinimum)
    );
    var maximum = this.readNumber(
        configuration.maximum,
        this.readNumber(valueModel.maximum, physicalMaximum)
    );
    var value = this.readNumber(valueSource, physicalMinimum);
    var hasDefaultValue = configuration.defaultValue !== undefined
        || valueModel.defaultValue !== undefined;
    var defaultValue = hasDefaultValue
        ? this.readNumber(
            configuration.defaultValue,
            this.readNumber(valueModel.defaultValue, value)
        )
        : null;
    var visualization = this.read(
        configuration.visualization,
        valueModel.visualization || null
    );

    var ring = {
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
};

DialPresenter.prototype.formatDisplayValue = function (value, display) {
    var scale = display.scale === undefined ? 1 : Number(display.scale);
    if (!isFinite(scale)) scale = 1;
    value *= scale;
    var decimals = display.decimals === undefined ? 2
        : Math.max(0, Math.floor(Number(display.decimals)));
    var suffix = display.suffix === undefined ? "" : String(display.suffix);
    return Number(value).toFixed(decimals) + suffix;
};

DialPresenter.prototype.buildVisualization = function (source) {
    var visualization = source || {};
    var type = visualization.type || "none";
    if (type === "none") return null;
    var range = visualization.range || {};
    var minimum = this.readNumber(range.minimum, 0);
    var maximum = this.readNumber(range.maximum, 1);
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
};

DialPresenter.prototype.clampRelative = function (value) {
    return Math.max(-1, Math.min(1, value));
};

DialPresenter.prototype.rebuild = function () {
    var presentation = new DialPresentation();
    presentation.enabled = this.read(this.options.enabled, true);
    presentation.active = this.read(this.options.active, true);

    var rings = this.options.rings || [];
    this.ringMappings = [];
    for (var index = 0; index < Math.min(rings.length, this.maximumRingCount); index += 1) {
        presentation.rings.push(this.buildRing(rings[index]));
    }
    presentation.activeIndex = this.normalizeIndex(
        this.state.activeIndex, presentation.rings.length
    );
    presentation.displayIndex = this.normalizeIndex(
        this.state.displayIndex, presentation.rings.length
    );
    this.publish(presentation);
};

DialPresenter.prototype.normalizeIndex = function (value, count) {
    var index = Number(this.read(value, 0));
    if (!isFinite(index) || count === 0) return 0;
    return Math.max(0, Math.min(count - 1, Math.floor(index)));
};

DialPresenter.prototype.setValue = function (ringIndex, normalizedValue) {
    var configuration = (this.options.rings || [])[ringIndex];
    var ring = this.presentation && this.presentation.rings[ringIndex];
    var mapping = this.ringMappings[ringIndex];
    if (!configuration || !ring || !mapping || !configuration.value) {
        return;
    }

    var normalizedValue = clampPresentationValue(
        normalizedValue, ring.minimum, ring.maximum
    );
    var physicalValue = denormalizePresentationValue(
        normalizedValue,
        mapping.physicalMinimum,
        mapping.physicalMaximum,
        mapping.logarithmic
    );
    var physicalStep = Number(mapping.physicalStep);
    if (isFinite(physicalStep) && physicalStep > 0) {
        physicalValue = mapping.physicalMinimum + Math.round(
            (physicalValue - mapping.physicalMinimum) / physicalStep
        ) * physicalStep;
    }
    var physicalMinimum = denormalizePresentationValue(
        ring.minimum,
        mapping.physicalMinimum,
        mapping.physicalMaximum,
        mapping.logarithmic
    );
    var physicalMaximum = denormalizePresentationValue(
        ring.maximum,
        mapping.physicalMinimum,
        mapping.physicalMaximum,
        mapping.logarithmic
    );
    physicalValue = clampPresentationValue(
        physicalValue, physicalMinimum, physicalMaximum
    );
    presentationBindingWrite(configuration.value, physicalValue);
};

DialPresenter.prototype.resetValue = function (ringIndex) {
    var configuration = (this.options.rings || [])[ringIndex];
    var source = configuration && presentationBindingSource(configuration.value);
    if (source && typeof source.reset === "function") {
        source.reset();
    }
};

DialPresenter.prototype.setActive = function (value) {
    presentationBindingWrite(this.options.active, value);
};

DialPresenter.prototype.setActiveIndex = function (index) {
    var count = this.presentation ? this.presentation.rings.length : 0;
    var nextIndex = this.normalizeIndex(index, count);
    this.state.activeIndex = nextIndex;
    this.rebuild();
};

DialPresenter.prototype.beginGesture = function (ringIndex) {
    this.emit("gestureBegan", { index: ringIndex });
};

DialPresenter.prototype.endGesture = function (ringIndex) {
    this.emit("gestureEnded", { index: ringIndex });
};

DialPresenter.prototype.destroy = function () {
    if (this.destroyed) return;
    for (var index = 0; index < this.unsubscribers.length; index += 1) {
        this.unsubscribers[index]();
    }
    this.unsubscribers = [];
    this.eventListeners = {};
    PresentationObservable.prototype.destroy.call(this);
};
