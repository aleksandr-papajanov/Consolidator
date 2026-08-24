include("Project:/js/Presenters/Core/PresentationObservable.js");
include("Project:/js/Presenters/Core/PresentationBinding.js");
include("Project:/js/Presenters/Core/Normalization.js");
include("Project:/js/Presenters/Slider/SliderPresentation.js");

function SliderPresenter(options) {
    PresentationObservable.call(this);
    this.options = options || {};
    this.mapping = null;
    this.unsubscribers = [];
    this.subscribeSources();
    this.rebuild();
}

SliderPresenter.prototype = Object.create(PresentationObservable.prototype);
SliderPresenter.prototype.constructor = SliderPresenter;

SliderPresenter.prototype.read = function (source, fallback) {
    return presentationBindingValue(source, fallback);
};

SliderPresenter.prototype.readNumber = function (source, fallback) {
    var value = this.read(source, fallback);
    return typeof value === "number" && isFinite(value) ? value : fallback;
};

SliderPresenter.prototype.subscribeSources = function () {
    var self = this;
    function changed() { self.rebuild(); }
    var sources = [this.options.value, this.options.minimum,
        this.options.maximum, this.options.physicalMinimum,
        this.options.physicalMaximum, this.options.physicalStep,
        this.options.enabled, this.options.active, this.options.color];
    for (var index = 0; index < sources.length; index += 1) {
        subscribePresentationBinding(sources[index], changed, this.unsubscribers);
    }
};

SliderPresenter.prototype.formatDisplayValue = function (value, display) {
    var decimals = display.decimals === undefined ? 2
        : Math.max(0, Math.floor(Number(display.decimals)));
    var suffix = display.suffix === undefined ? "" : String(display.suffix);
    return Number(value).toFixed(decimals) + suffix;
};

SliderPresenter.prototype.rebuild = function () {
    var valueSource = this.options.value;
    var valueModel = valueSource && valueSource.value !== undefined
        ? valueSource : {};
    var physicalMinimum = this.readNumber(
        this.options.physicalMinimum,
        this.readNumber(valueModel.physicalMinimum, 0)
    );
    var physicalMaximum = this.readNumber(
        this.options.physicalMaximum,
        this.readNumber(valueModel.physicalMaximum, 1)
    );
    var minimum = this.readNumber(
        this.options.minimum,
        this.readNumber(valueModel.minimum, physicalMinimum)
    );
    var maximum = this.readNumber(
        this.options.maximum,
        this.readNumber(valueModel.maximum, physicalMaximum)
    );
    var value = this.readNumber(valueSource, physicalMinimum);
    var display = this.options.display || valueModel.display || {};
    var mapping = this.options.mapping || {};
    var logarithmic = mapping.type === "logarithmic";

    this.mapping = {
        physicalMinimum: physicalMinimum,
        physicalMaximum: physicalMaximum,
        physicalStep: this.readNumber(
            this.options.physicalStep,
            this.readNumber(valueModel.physicalStep, 0)
        ),
        logarithmic: logarithmic
    };

    var presentation = new SliderPresentation();
    presentation.value = normalizePresentationValue(
        value, physicalMinimum, physicalMaximum, logarithmic
    );
    presentation.minimum = normalizePresentationValue(
        minimum, physicalMinimum, physicalMaximum, logarithmic
    );
    presentation.maximum = normalizePresentationValue(
        maximum, physicalMinimum, physicalMaximum, logarithmic
    );
    presentation.enabled = this.read(this.options.enabled, true);
    presentation.active = this.read(this.options.active, true);
    presentation.orientation = this.options.orientation === "vertical"
        ? "vertical" : "horizontal";
    presentation.display = {
        value: this.formatDisplayValue(value, display)
    };
    presentation.color = this.read(
        this.options.color,
        valueModel.color || null
    );
    this.publish(presentation);
};

SliderPresenter.prototype.setValue = function (normalizedValue) {
    var source = this.options.value;
    if (!source || !this.mapping) return;
    var value = clampPresentationValue(
        normalizedValue,
        this.presentation.minimum,
        this.presentation.maximum
    );
    var physicalValue = denormalizePresentationValue(
        value,
        this.mapping.physicalMinimum,
        this.mapping.physicalMaximum,
        this.mapping.logarithmic
    );
    var step = Number(this.mapping.physicalStep);
    if (isFinite(step) && step > 0) {
        physicalValue = this.mapping.physicalMinimum + Math.round(
            (physicalValue - this.mapping.physicalMinimum) / step
        ) * step;
    }
    var physicalMinimum = denormalizePresentationValue(
        this.presentation.minimum,
        this.mapping.physicalMinimum,
        this.mapping.physicalMaximum,
        this.mapping.logarithmic
    );
    var physicalMaximum = denormalizePresentationValue(
        this.presentation.maximum,
        this.mapping.physicalMinimum,
        this.mapping.physicalMaximum,
        this.mapping.logarithmic
    );
    presentationBindingWrite(this.options.value, clampPresentationValue(
        physicalValue, physicalMinimum, physicalMaximum
    ));
};

SliderPresenter.prototype.resetValue = function () {
    var source = presentationBindingSource(this.options.value);
    if (source && typeof source.reset === "function") source.reset();
};

SliderPresenter.prototype.destroy = function () {
    if (this.destroyed) return;
    for (var index = 0; index < this.unsubscribers.length; index += 1) {
        this.unsubscribers[index]();
    }
    this.unsubscribers = [];
    PresentationObservable.prototype.destroy.call(this);
};
