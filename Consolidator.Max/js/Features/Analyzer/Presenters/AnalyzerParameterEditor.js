const {
    presentationBindingValue,
    presentationBindingWrite
} = require("../../../Shared/Presenters/PresentationBinding.js");

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

        const gain = this.scale.clampBindingValue(
            parameter.gain,
            this.scale.yToGain(y)
        );
        const frequency = parameter.frequency
            ? this.scale.clampBindingValue(
                parameter.frequency,
                this.scale.xToFrequency(x)
            )
            : null;
        this.preview[Number(id)] = { frequency: frequency, gain: gain };
        this.requestRebuild();
        if (typeof parameter.setPosition === "function")
        {
            parameter.setPosition(frequency, gain, transactionId);
            return;
        }
        if (parameter.frequency && frequency !== null)
        {
            presentationBindingWrite(parameter.frequency, frequency, transactionId);
        }
        presentationBindingWrite(parameter.gain, gain, transactionId);
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
        const next = Math.max(range.minimum, Math.min(range.maximum, current + change));
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
        const preview = {
            gain: this.scale.clampBindingValue(
                parameter.gain,
                this.scale.yToGain(y)
            )
        };
        if (parameter.frequency)
        {
            preview.frequency = this.scale.clampBindingValue(
                parameter.frequency,
                this.scale.xToFrequency(x)
            );
        }
        return preview;
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
