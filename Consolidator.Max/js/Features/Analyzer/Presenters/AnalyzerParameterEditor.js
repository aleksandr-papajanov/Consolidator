const {
    presentationBindingValue,
    presentationBindingWrite
} = require("../../../Shared/Presenters/PresentationBinding.js");
const { quantizeRough } = require(
    "../../../Shared/Controls/Dial/DialValueEditor.js");
const { uiEditPrecision } = require(
    "../../../Shared/ViewModels/UiEditPrecision.js");

const Q_ROUGH_STEP_COUNT = 5;
const Q_ROUGH_DRAG_SENSITIVITY = 0.01;
const Q_RANGE_EPSILON = 1e-9;
const FREQUENCY_ROUGH_STEPS_PER_DECADE = 10;

function quantizeQLogarithmic(value, minimum, maximum)
{
    const intervals = Q_ROUGH_STEP_COUNT - 1;
    const exponent = Math.log(value / minimum) / Math.log(maximum / minimum);
    const step = Math.round(exponent * intervals) / intervals;
    return minimum * Math.pow(maximum / minimum, step);
}

function quantizeFrequencyLogarithmic(value, gridMinimum)
{
    if (value <= 0 || gridMinimum <= 0) return value;
    const step = Math.round(
        Math.log10(value / gridMinimum) * FREQUENCY_ROUGH_STEPS_PER_DECADE);
    return gridMinimum * Math.pow(
        10,
        step / FREQUENCY_ROUGH_STEPS_PER_DECADE
    );
}

class AnalyzerParameterEditor
{
    constructor(parameters, scale, isReady, requestRebuild)
    {
        this.parameters = parameters || [];
        this.scale = scale;
        this.isReady = isReady;
        this.requestRebuild = requestRebuild;
        this.preview = {};
        this.qDragValue = {};
        this.gestureActive = false;
        this.precisionMode = uiEditPrecision;
    }

    sourceChanged()
    {
        if (!this.gestureActive)
        {
            this.preview = {};
        }
    }

    previewMoved(id, x, y)
    {
        const parameter = this.parameter(id);
        if (!parameter || !parameter.gain || !this.isReady())
        {
            return;
        }

        this.preview[Number(id)] = this.createPositionPreview(parameter, x, y);
        this.requestRebuild();
    }

    beginGesture()
    {
        this.preview = {};
        this.qDragValue = {};
        this.gestureActive = true;
    }

    endGesture()
    {
        if (!this.gestureActive && Object.keys(this.preview).length === 0)
        {
            return;
        }
        this.gestureActive = false;
        this.preview = {};
        this.qDragValue = {};
        this.requestRebuild();
    }

    move(id, x, y, transactionId)
    {
        const parameter = this.parameter(id);
        if (!parameter || !parameter.gain || !this.isReady())
        {
            return;
        }

        const position = this.createPositionPreview(parameter, x, y);
        const quantizedGain = position.gain;
        const quantizedFrequency = position.frequency;
        this.preview[Number(id)] = position;
        this.requestRebuild();
        if (typeof parameter.setPosition === "function")
        {
            parameter.setPosition(quantizedFrequency, quantizedGain, transactionId);
            return;
        }
        if (parameter.frequency && quantizedFrequency !== null)
        {
            presentationBindingWrite(parameter.frequency, quantizedFrequency, transactionId);
        }
        presentationBindingWrite(parameter.gain, quantizedGain, transactionId);
    }

    reset(id, callback)
    {
        const parameter = this.parameter(id);
        if (!parameter || !this.isReady() || typeof parameter.reset !== "function")
        {
            if (callback)
            {
                callback({ status: "accepted", error: null });
            }
            return;
        }
        parameter.reset(callback);
        this.preview[Number(id)] = null;
        this.requestRebuild();
    }

    commit(id, x, y, transactionId, callback)
    {
        if (isFinite(Number(x)) && isFinite(Number(y)))
        {
            const parameter = this.parameter(id);
            if (parameter && parameter.gain && this.isReady())
            {
                this.preview[Number(id)] = this.createPositionPreview(parameter, x, y);
            }
        }
        const preview = this.preview[Number(id)];
        const parameter = this.parameter(id);
        if (!preview || preview.gain === undefined || !parameter ||
                !parameter.gain || parameter.frequency && preview.frequency === undefined)
        {
            this.completeAccepted(callback);
            return;
        }
        if (typeof parameter.setPosition === "function")
        {
            parameter.setPosition(
                preview.frequency,
                preview.gain,
                transactionId,
                callback
            );
            return;
        }
        if (parameter.frequency)
        {
            presentationBindingWrite(
                parameter.frequency,
                preview.frequency,
                transactionId
            );
        }
        presentationBindingWrite(parameter.gain, preview.gain, transactionId);
        this.completeAccepted(callback);
    }

    changeQ(id, delta, transactionId)
    {
        this.updateQ(id, delta, transactionId, true);
    }

    previewQ(id, delta)
    {
        this.updateQ(id, delta, null, false);
    }

    commitQ(id, transactionId)
    {
        const parameter = this.parameter(id);
        const preview = this.preview[Number(id)];
        if (!parameter || !parameter.q || !preview ||
                preview.q === undefined || !this.isReady())
        {
            return;
        }
        presentationBindingWrite(parameter.q, preview.q, transactionId);
    }

    updateQ(id, delta, transactionId, write)
    {
        const parameter = this.parameter(id);
        if (!parameter || !parameter.q || !this.isReady())
        {
            return;
        }

        const range = this.scale.bindingRange(parameter.q, 0.01, 10);
        if (range.minimum <= 0 || range.maximum <= range.minimum)
        {
            return;
        }
        const preview = this.preview[Number(id)];
        const key = Number(id);
        let current = this.qDragValue[key] !== undefined
            ? this.qDragValue[key]
            : preview && preview.q !== undefined
                ? Number(preview.q) : Number(presentationBindingValue(parameter.q));
        const change = Number(delta) * Q_ROUGH_DRAG_SENSITIVITY;
        const rough = this.precisionMode.isRough();
        if (!isFinite(current) || !isFinite(change) ||
                current < range.minimum - Q_RANGE_EPSILON ||
                current > range.maximum + Q_RANGE_EPSILON)
        {
            return;
        }
        current = Math.max(range.minimum, Math.min(range.maximum, current));
        let rawNext = Math.max(range.minimum, Math.min(
            range.maximum, current + change));
        let next = rawNext;
        if (rough)
        {
            rawNext = current * Math.pow(
                range.maximum / range.minimum,
                Number(delta) * Q_ROUGH_DRAG_SENSITIVITY);
            rawNext = Math.max(range.minimum, Math.min(
                range.maximum, rawNext));
            next = Math.max(range.minimum, Math.min(
                range.maximum, quantizeQLogarithmic(
                    rawNext, range.minimum, range.maximum)));
            if (current <= range.minimum && Number(delta) > 0)
            {
                next = range.minimum * Math.pow(
                    range.maximum / range.minimum,
                    1 / (Q_ROUGH_STEP_COUNT - 1));
                rawNext = next;
            }
            else if (current >= range.maximum && Number(delta) < 0)
            {
                next = range.maximum * Math.pow(
                    range.minimum / range.maximum,
                    1 / (Q_ROUGH_STEP_COUNT - 1));
                rawNext = next;
            }
        }
        this.qDragValue[key] = rawNext;
        this.preview[key] = { q: next };
        this.requestRebuild();
        if (write)
        {
            presentationBindingWrite(parameter.q, next, transactionId);
        }
    }

    parameter(id)
    {
        return this.parameters[Number(id) - 1];
    }

    createPositionPreview(parameter, x, y)
    {
        let gain = this.scale.clampBindingValue(
                parameter.gain,
                this.scale.yToGain(y)
            );
        let frequency = null;
        if (parameter.frequency)
        {
            frequency = this.scale.clampBindingValue(
                parameter.frequency,
                this.scale.xToFrequency(x)
            );
        }
        if (this.precisionMode.isRough())
        {
            gain = this.scale.clampBindingValue(
                parameter.gain,
                quantizeRough(gain, true)
            );
            if (frequency !== null)
            {
                frequency = quantizeFrequencyLogarithmic(
                    frequency, this.scale.frequencyMinimum);
                frequency = this.scale.clampBindingValue(
                    parameter.frequency, frequency);
            }
        }
        return { frequency: frequency, gain: gain };
    }

    completeAccepted(callback)
    {
        if (callback)
        {
            callback({ status: "accepted", error: null });
        }
    }

    clear()
    {
        this.preview = {};
        this.qDragValue = {};
        this.gestureActive = false;
    }
}

module.exports = {
    AnalyzerParameterEditor: AnalyzerParameterEditor
};
