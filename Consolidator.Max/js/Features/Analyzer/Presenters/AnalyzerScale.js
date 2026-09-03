const { presentationBindingSource } = require(
    "../../../Shared/Presenters/PresentationBinding.js");

class AnalyzerScale
{
    constructor(frequencyRange, gainRange)
    {
        frequencyRange = frequencyRange || {};
        gainRange = gainRange || {};
        this.frequencyMinimum = frequencyRange.minimum === undefined
            ? 20
            : Number(frequencyRange.minimum);
        this.frequencyMaximum = frequencyRange.maximum === undefined
            ? 20000
            : Number(frequencyRange.maximum);
        this.gainMinimum = gainRange.minimum === undefined
            ? -24
            : Number(gainRange.minimum);
        this.gainMaximum = gainRange.maximum === undefined
            ? 24
            : Number(gainRange.maximum);
    }

    frequencyToX(value)
    {
        const minimum = Math.log(this.frequencyMinimum);
        return (Math.log(Math.max(this.frequencyMinimum, Number(value))) - minimum) /
            (Math.log(this.frequencyMaximum) - minimum);
    }

    xToFrequency(x)
    {
        const position = Math.max(0, Math.min(1, Number(x)));
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
        const position = Math.max(0, Math.min(1, Number(y)));
        return this.gainMinimum + (1 - position) *
            (this.gainMaximum - this.gainMinimum);
    }

    clampBindingValue(binding, value)
    {
        const range = this.bindingRange(binding);

        let result = Number(value);
        if (isFinite(range.minimum))
        {
            result = Math.max(range.minimum, result);
        }
        if (isFinite(range.maximum))
        {
            result = Math.min(range.maximum, result);
        }
        return result;
    }

    bindingRange(binding, defaultMinimum, defaultMaximum)
    {
        const source = presentationBindingSource(binding);
        let minimum = Number(source && source.minimum);
        if (!isFinite(minimum))
        {
            minimum = Number(source && source.physicalMinimum);
        }
        if (!isFinite(minimum))
        {
            minimum = defaultMinimum;
        }
        let maximum = Number(source && source.maximum);
        if (!isFinite(maximum))
        {
            maximum = Number(source && source.physicalMaximum);
        }
        if (!isFinite(maximum))
        {
            maximum = defaultMaximum;
        }
        return { minimum: minimum, maximum: maximum };
    }
}

module.exports = {
    AnalyzerScale: AnalyzerScale
};
