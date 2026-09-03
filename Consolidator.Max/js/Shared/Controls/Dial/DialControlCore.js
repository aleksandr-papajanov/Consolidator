const { DialPresentation } = require(
    "./DialPresentation.js");
const { DoubleClickTracker } = require("../DoubleClickTracker.js");
const { DialControlOptions } = require("./DialControlOptions.js");
const { DialRenderer, clampDialValue } = require("./DialRenderer.js");

class DialControlCore
{
    constructor(configuration, taskConstructor)
    {
        this.presentation = new DialPresentation();
        this.presentation.rings.push(this.createDefaultRing(configuration));
        this.presentation.label = configuration.label;
        this.previewValues = [];
        this.dragging = false;
        this.dragIndex = 0;
        this.lastY = 0;
        this.presentationBatch = false;
        this.doubleClick = new DoubleClickTracker();
        this.showValueLabel = false;
        this.renderer = new DialRenderer();
        this.destroyed = false;
        this.labelRestoreTask = new taskConstructor(() => {
            if (this.destroyed) return;
            this.showValueLabel = false;
            this.requestRedraw();
        }, this);
    }

    createDefaultRing(configuration)
    {
        return {
            value: 0,
            minimum: 0,
            maximum: 1,
            visualization: null,
            color: null,
            display: {
                minimum: configuration.minimum,
                maximum: configuration.maximum,
                logarithmic: configuration.logarithmic,
                scale: configuration.scale,
                decimals: configuration.decimals,
                suffix: configuration.suffix === "dB"
                    ? " dB" : configuration.suffix
            }
        };
    }

    applyPresentation(presentation)
    {
        if (!presentation) return;
        let previewIndex = this.dragging ? this.dragIndex : -1;
        let previewValue = previewIndex >= 0
            ? this.previewValues[previewIndex] : undefined;
        this.presentation = presentation;
        this.previewValues = [];
        if (previewIndex >= 0 && previewValue !== undefined &&
                this.presentation.rings[previewIndex]) {
            this.previewValues[previewIndex] = previewValue;
        }
        if (!this.presentation.enabled) {
            this.dragging = false;
            this.previewValues = [];
        }
        this.requestRedraw();
    }

    beginPresentation() { this.presentationBatch = true; }

    endPresentation()
    {
        if (!this.presentationBatch) return;
        this.presentationBatch = false;
        mgraphics.redraw();
    }

    requestRedraw()
    {
        if (!this.presentationBatch && !this.destroyed) mgraphics.redraw();
    }

    setPresentationValue(index, value)
    {
        let ring = this.presentation.rings[index];
        if (!ring) return;
        ring.value = clampDialValue(Number(value), ring.minimum, ring.maximum);
        if (!this.dragging || this.dragIndex !== index) {
            delete this.previewValues[index];
        }
        this.requestRedraw();
    }

    setPresentationLimits(index, minimum, maximum)
    {
        let ring = this.presentation.rings[index];
        if (!ring) return;
        ring.minimum = Number(minimum);
        ring.maximum = Number(maximum);
        this.requestRedraw();
    }

    setScope(active, hasColor, red, green, blue, alpha)
    {
        this.presentation.groupScope = Number(active) !== 0;
        this.presentation.scopeActive = Number(active) !== 0;
        this.presentation.scopeColor = Number(hasColor) !== 0
            ? [Number(red), Number(green), Number(blue), Number(alpha)] : null;
        this.requestRedraw();
    }

    clamp(value, minimum, maximum)
    {
        return clampDialValue(value, minimum, maximum);
    }

    emit(name, payload)
    {
        if (payload === undefined) outlet(0, name);
        else if (payload instanceof Array) outlet(0, [name].concat(payload));
        else outlet(0, [name, payload]);
    }

    setValue(index, value, output)
    {
        let ring = this.presentation.rings[index];
        if (!ring) return;
        let next = clampDialValue(Number(value), ring.minimum, ring.maximum);
        if (!isFinite(next)) return;
        let current = this.previewValues[index] === undefined
            ? ring.value : this.previewValues[index];
        if (Math.abs(next - current) < 0.0000001) return;
        this.previewValues[index] = next;
        this.requestRedraw();
        if (output) this.emit("valueChanged", [index, next]);
    }

    resetValue(index)
    {
        if (this.presentation.rings[index]) this.emit("reset", index);
    }

    displayValue(ring, value)
    {
        return this.renderer.displayValue(ring, value);
    }

    paint()
    {
        this.renderer.paint(
            this.presentation, this.previewValues, this.showValueLabel);
    }

    beginGesture(index, y)
    {
        if (!this.presentation.enabled || !this.presentation.active ||
                !this.presentation.rings[index]) return;
        if (this.doubleClick.isDoubleClick(index)) {
            this.endGesture();
            this.previewValues = [];
            this.emit("reset", index);
            this.requestRedraw();
            return;
        }
        this.dragging = true;
        this.dragIndex = index;
        this.lastY = y;
        this.labelRestoreTask.cancel();
        this.showValueLabel = true;
        this.requestRedraw();
        this.emit("gestureBegan", index);
    }

    drag(y)
    {
        if (!this.dragging) return;
        let ring = this.presentation.rings[this.dragIndex];
        let value = this.previewValues[this.dragIndex] === undefined
            ? ring.value : this.previewValues[this.dragIndex];
        this.setValue(this.dragIndex,
            value + (this.lastY - y) * DialControlOptions.dragSensitivity, true);
        this.lastY = y;
    }

    endGesture()
    {
        if (!this.dragging) return;
        this.dragging = false;
        this.emit("gestureEnded", this.dragIndex);
        this.labelRestoreTask.schedule(DialControlOptions.labelRestoreDelayMs);
    }

    rejectTransaction()
    {
        this.dragging = false;
        this.previewValues = [];
        this.labelRestoreTask.cancel();
        this.showValueLabel = false;
        this.requestRedraw();
    }

    destroy()
    {
        this.destroyed = true;
        this.dragging = false;
        this.previewValues = [];
        this.labelRestoreTask.cancel();
    }
}

module.exports = {
    DialControlCore: DialControlCore
};
