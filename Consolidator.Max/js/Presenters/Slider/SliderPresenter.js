const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { presentationBindingSource } = require("../Core/PresentationBinding.js");
const { presentationBindingValue } = require("../Core/PresentationBinding.js");
const { presentationBindingWrite } = require("../Core/PresentationBinding.js");
const { subscribePresentationBinding } = require("../Core/PresentationBinding.js");
const { clampPresentationValue } = require("../Core/Normalization.js");
const { normalizePresentationValue } = require("../Core/Normalization.js");
const { denormalizePresentationValue } = require("../Core/Normalization.js");
const { SliderPresentation } = require("./SliderPresentation.js");

class SliderPresenter extends PresentationObservable
{
    constructor(options)
    {
        super();
        this.options = options || {};
        this.mapping = null;
        this.unsubscribers = [];
        this.subscribeSources();
        this.rebuild();
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
    
    subscribeSources()
    {
        const changed = () => { this.requestRebuild(); };
        const sources = [this.options.value, this.options.minimum,
            this.options.maximum, this.options.physicalMinimum,
            this.options.physicalMaximum, this.options.physicalStep,
            this.options.enabled, this.options.active, this.options.color];
        for (let index = 0; index < sources.length; index += 1) {
            subscribePresentationBinding(sources[index], changed, this.unsubscribers);
        }
    }
    
    formatDisplayValue(value, display)
    {
        let decimals = display.decimals === undefined ? 2
            : Math.max(0, Math.floor(Number(display.decimals)));
        let suffix = display.suffix === undefined ? "" : String(display.suffix);
        return Number(value).toFixed(decimals) + suffix;
    }
    
    rebuild()
    {
        let valueSource = this.options.value;
        let valueModel = valueSource && valueSource.value !== undefined
            ? valueSource : {};
        let physicalMinimum = this.readNumber(
            this.options.physicalMinimum,
            this.readNumber(valueModel.physicalMinimum, 0)
        );
        let physicalMaximum = this.readNumber(
            this.options.physicalMaximum,
            this.readNumber(valueModel.physicalMaximum, 1)
        );
        let minimum = this.readNumber(
            this.options.minimum,
            this.readNumber(valueModel.minimum, physicalMinimum)
        );
        let maximum = this.readNumber(
            this.options.maximum,
            this.readNumber(valueModel.maximum, physicalMaximum)
        );
        let value = this.readNumber(valueSource, physicalMinimum);
        let display = this.options.display || valueModel.display || {};
        let mapping = this.options.mapping || {};
        let logarithmic = mapping.type === "logarithmic";
    
        this.mapping = {
            physicalMinimum: physicalMinimum,
            physicalMaximum: physicalMaximum,
            physicalStep: this.readNumber(
                this.options.physicalStep,
                this.readNumber(valueModel.physicalStep, 0)
            ),
            logarithmic: logarithmic
        };
    
        let presentation = new SliderPresentation();
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
    }
    
    setValue(normalizedValue)
    {
        let source = this.options.value;
        if (!source || !this.mapping) return;
        let value = clampPresentationValue(
            normalizedValue,
            this.presentation.minimum,
            this.presentation.maximum
        );
        let physicalValue = denormalizePresentationValue(
            value,
            this.mapping.physicalMinimum,
            this.mapping.physicalMaximum,
            this.mapping.logarithmic
        );
        let step = Number(this.mapping.physicalStep);
        if (isFinite(step) && step > 0) {
            physicalValue = this.mapping.physicalMinimum + Math.round(
                (physicalValue - this.mapping.physicalMinimum) / step
            ) * step;
        }
        let physicalMinimum = denormalizePresentationValue(
            this.presentation.minimum,
            this.mapping.physicalMinimum,
            this.mapping.physicalMaximum,
            this.mapping.logarithmic
        );
        let physicalMaximum = denormalizePresentationValue(
            this.presentation.maximum,
            this.mapping.physicalMinimum,
            this.mapping.physicalMaximum,
            this.mapping.logarithmic
        );
        presentationBindingWrite(this.options.value, clampPresentationValue(
            physicalValue, physicalMinimum, physicalMaximum
        ));
    }
    
    resetValue()
    {
        let source = presentationBindingSource(this.options.value);
        if (source && typeof source.reset === "function") source.reset();
    }
    
    destroy()
    {
        if (this.destroyed) return;
        for (let index = 0; index < this.unsubscribers.length; index += 1) {
            this.unsubscribers[index]();
        }
        this.unsubscribers = [];
        super.destroy();
    }
}


module.exports = {
    SliderPresenter: SliderPresenter
};
