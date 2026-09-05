const {
    presentationBindingValue,
    presentationBindingWrite
} = require("../../../Shared/Presenters/PresentationBinding.js");
const { quantizeRough, quantizeLog } = require(
    "../../../Shared/Controls/Dial/DialValueEditor.js");
const { uiEditPrecision } = require(
    "../../../Shared/ViewModels/UiEditPrecision.js");

class AnalyzerParameterEditor
{
    constructor(parameters, scale, isReady, requestRebuild)
    {
        this.parameters = parameters || [];
        this.scale = scale;
        this.isReady = isReady;
        this.requestRebuild = requestRebuild;
        this.preview = {};
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

    changeQ(id, delta)
    {
        const parameter = this.parameter(id);
        if (!parameter || !parameter.q || !this.isReady())
        {
            return;
        }

        const range = this.scale.bindingRange(parameter.q, 0.01, 10);
        if (range.maximum < range.minimum)
        {
            return;
        }
        const current = Number(presentationBindingValue(parameter.q));
        const change = Number(delta);
        if (!isFinite(current) || !isFinite(change) ||
                current < range.minimum || current > range.maximum)
        {
            return;
        }
        let next = Math.max(range.minimum, Math.min(range.maximum, current + change));
        if (this.precisionMode.isRough())
        {
            next = quantizeLog(next);
        }
        this.preview[Number(id)] = { q: next };
        this.requestRebuild();
        presentationBindingWrite(parameter.q, next);
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
            gain = quantizeRough(gain, true);
            if (frequency !== null)
            {
                frequency = quantizeLog(frequency);
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
        this.gestureActive = false;
    }
}

module.exports = {
    AnalyzerParameterEditor: AnalyzerParameterEditor
};
