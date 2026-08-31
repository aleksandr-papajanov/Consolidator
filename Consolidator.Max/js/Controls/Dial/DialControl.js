autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { DialPresentation } = require("../../Presenters/Dial/DialPresentation.js");
const { DoubleClickTracker } = require("../DoubleClickTracker.js");
const { UiColors } = require("../../Theme/UiColors.js");

function dialArgument(index, fallback)
{
    return jsarguments.length > index ? jsarguments[index] : fallback;
}

const DialControlConfiguration = {
    label: String(dialArgument(1, "")),
    minimum: Number(dialArgument(2, 0)),
    maximum: Number(dialArgument(3, 1)),
    logarithmic: Number(dialArgument(4, 0)) !== 0,
    scale: Number(dialArgument(5, 1)),
    decimals: Math.max(0, Math.floor(Number(dialArgument(6, 2)))),
    suffix: String(dialArgument(7, ""))
};

const DialControlOptions = {
    startAngle: Math.PI * (5 / 6),
    endAngle: Math.PI * (13 / 6),
    lineWidth: 3,
    indicatorWidth: 2,
    dragSensitivity: 0.007,
    labelRestoreDelayMs: 500,
    background: UiColors.base.background,
    ring: UiColors.controls.ring,
    active: UiColors.base.brightText,
    inactive: UiColors.controls.inactive,
    visualization: UiColors.controls.visualization
};

class DialControl
{
    constructor()
    {
        this.presentation = new DialPresentation();
        this.presentation.rings.push({
            value: 0,
            minimum: 0,
            maximum: 1,
            visualization: null,
            color: null,
            display: {
                minimum: DialControlConfiguration.minimum,
                maximum: DialControlConfiguration.maximum,
                logarithmic: DialControlConfiguration.logarithmic,
                scale: DialControlConfiguration.scale,
                decimals: DialControlConfiguration.decimals,
                suffix: DialControlConfiguration.suffix === "dB"
                    ? " dB" : DialControlConfiguration.suffix
            }
        });
        this.presentation.label = DialControlConfiguration.label;
        this.previewValues = [];
        this.dragging = false;
        this.dragIndex = 0;
        this.lastY = 0;
        this.presentationBatch = false;
        this.doubleClick = new DoubleClickTracker();
        this.showValueLabel = false;
        this.labelRestoreTask = new Task(() => {
            this.showValueLabel = false;
            this.requestRedraw();
        }, this);
    }

    applyPresentation(presentation)
    {
        if (!presentation) return;
        let previewIndex = this.dragging ? this.dragIndex : -1;
        let previewValue = previewIndex >= 0
            ? this.previewValues[previewIndex] : undefined;
        this.presentation = presentation;
        this.previewValues = [];
        if (previewIndex >= 0 && previewValue !== undefined
                && this.presentation.rings[previewIndex]) {
            this.previewValues[previewIndex] = previewValue;
        }
        if (!this.presentation.enabled) {
            this.dragging = false;
            this.previewValues = [];
        }
        this.requestRedraw();
    }

    beginPresentation()
    {
        this.presentationBatch = true;
    }

    endPresentation()
    {
        if (!this.presentationBatch) return;
        this.presentationBatch = false;
        mgraphics.redraw();
    }

    requestRedraw()
    {
        if (!this.presentationBatch) mgraphics.redraw();
    }

    setPresentationValue(index, value)
    {
        let ring = this.presentation.rings[index];
        if (!ring) return;
        ring.value = this.clamp(Number(value), ring.minimum, ring.maximum);
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
        mgraphics.redraw();
    }

    clamp(value, minimum, maximum)
    {
        return Math.max(minimum, Math.min(maximum, value));
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
        let next = this.clamp(Number(value), ring.minimum, ring.maximum);
        if (!isFinite(next)) return;
        let currentValue = this.previewValues[index] === undefined
            ? ring.value : this.previewValues[index];
        if (Math.abs(next - currentValue) < 0.0000001) return;
        this.previewValues[index] = next;
        this.requestRedraw();
        if (output) this.emit("valueChanged", [index, next]);
    }

    resetValue(index)
    {
        let ring = this.presentation.rings[index];
        if (!ring) return;
        this.emit("reset", index);
    }

    color(color, fallback)
    {
        return color && color.length >= 4 ? color : fallback;
    }

    displayValue(ring, normalizedValue)
    {
        let display = ring.display || {};
        let minimum = Number(display.minimum);
        let maximum = Number(display.maximum);
        let value = Number(normalizedValue);
        if (!isFinite(minimum) || !isFinite(maximum) || !isFinite(value)) {
            return display.value || "";
        }
        value = this.clamp(value, 0, 1);
        let physicalValue = display.logarithmic && minimum > 0 && maximum > 0
            ? minimum * Math.pow(maximum / minimum, value)
            : minimum + (maximum - minimum) * value;
        let scale = Number(display.scale);
        let decimals = Number(display.decimals);
        if (!isFinite(scale)) scale = 1;
        if (!isFinite(decimals)) decimals = 2;
        return (physicalValue * scale).toFixed(Math.max(0, Math.floor(decimals))) +
            String(display.suffix || "");
    }

    arc(centerX, centerY, radius, value, color, width)
    {
        this.arcRange(centerX, centerY, radius, 0, value, color, width);
    }

    arcRange(centerX, centerY, radius, startValue, endValue, color, width)
    {
        color = this.color(color, DialControlOptions.ring);
        let start = DialControlOptions.startAngle;
        let angleRange = DialControlOptions.endAngle - start;
        let begin = start + angleRange * this.clamp(startValue, 0, 1);
        let end = start + angleRange * this.clamp(endValue, 0, 1);
        mgraphics.set_source_rgba.apply(mgraphics, color);
        mgraphics.set_line_width(width);
        mgraphics.new_path();
        mgraphics.arc(centerX, centerY, radius, begin, end);
        mgraphics.stroke();
    }

    paintRing(ring, value, index, centerX, centerY, radius)
    {
        let color = this.color(ring.color,
            this.presentation.enabled && this.presentation.active
            ? DialControlOptions.active : DialControlOptions.inactive);
        if (this.presentation.scopeColor &&
                this.presentation.scopeColor.length >= 4) {
            color = this.presentation.scopeColor;
        }
        if (!this.presentation.groupScope) {
            this.arc(centerX, centerY, radius, 1, DialControlOptions.ring,
                DialControlOptions.lineWidth);
            this.arc(centerX, centerY, radius, value, color,
                DialControlOptions.lineWidth);
            this.paintVisualization(ring.visualization, centerX, centerY, radius);
            return;
        }

        let minimum = this.clamp(Number(ring.minimum), 0, 1);
        let maximum = this.clamp(Number(ring.maximum), 0, 1);
        let nextValue = this.clamp(Number(value), minimum, maximum);
        if (maximum > minimum) {
            this.arcRange(centerX, centerY, radius, minimum, maximum,
                DialControlOptions.ring, DialControlOptions.lineWidth);
            this.arcRange(centerX, centerY, radius, minimum, nextValue,
                color, DialControlOptions.lineWidth);
        }

        this.paintVisualization(ring.visualization, centerX, centerY, radius);
    }

    paintVisualization(visualization, centerX, centerY, radius)
    {
        if (!visualization) return;
        let value;
        switch (visualization.type) {
        case "level":
            this.arc(
                centerX,
                centerY,
                radius + DialControlOptions.indicatorWidth * 2,
                this.clamp(Number(visualization.peak), 0, 1),
                DialControlOptions.visualization,
                DialControlOptions.indicatorWidth
            );
            value = visualization.smoothed;
            break;
        case "reduction":
            value = visualization.value;
            break;
        case "saturation":
            value = visualization.value;
            break;
        case "relative":
            value = Math.abs(visualization.value);
            break;
        default:
            return;
        }
        this.arc(
            centerX,
            centerY,
            radius + DialControlOptions.indicatorWidth,
            this.clamp(Number(value), 0, 1),
            DialControlOptions.visualization,
            DialControlOptions.indicatorWidth
        );
    }

    paint()
    {
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let centerX = width * 0.5;
        let hasLabel = Boolean(this.presentation.label);
        let centerY = height * 0.55;
        let radius = Math.max(1, Math.min(width, height) * 0.38);
        let rings = this.presentation.rings || [];

        mgraphics.set_source_rgba.apply(mgraphics, DialControlOptions.background);
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();
        let ring = rings[0];
        if (!ring) return;
        let value = this.previewValues[0] === undefined
            ? ring.value : this.previewValues[0];
        this.paintRing(ring, value, 0, centerX, centerY, radius);
        if (hasLabel) {
            mgraphics.select_font_face(
                UiColors.typography.controlLabelFontFamily);
            mgraphics.set_font_size(UiColors.typography.controlLabelFontSize);
            mgraphics.set_source_rgba.apply(mgraphics,
                UiColors.base.inactiveText);
            let label = this.showValueLabel && ring.display
                ? this.displayValue(ring, value)
                : String(this.presentation.label);
            let textSize = mgraphics.text_measure(label);
            mgraphics.move_to((width - textSize[0]) * 0.5, height - 7);
            mgraphics.show_text(label);
        }
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
        this.setValue(
            this.dragIndex,
            value + (this.lastY - y) * DialControlOptions.dragSensitivity,
            true
        );
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
}

function applyPresentation(presentation) {
    dialControl.applyPresentation(presentation);
}

function presentation(presentation) {
    applyPresentation(presentation);
}

function presentation_begin() {
    dialControl.beginPresentation();
}

function presentation_end() {
    dialControl.endPresentation();
}

function paint() {
    dialControl.paint();
}

function onresize() {
    mgraphics.redraw();
}

function onclick(x, y, button, mod1, shift, caps, opt, mod2) {
    dialControl.beginGesture(0, y);
}

function ondrag(x, y, button) {
    if (button === 0) dialControl.endGesture();
    else dialControl.drag(y);
}

function onidleout() {
    dialControl.endGesture();
}

function setValue(index, value) {
    dialControl.setValue(Number(index), Number(value), false);
}

function resetValue(index) {
    dialControl.resetValue(Number(index), "local");
}

function transactionRejected() {
    dialControl.rejectTransaction();
}

function set(index, value) {
    dialControl.setPresentationValue(Number(index), Number(value));
}

function limits(index, minimum, maximum) {
    dialControl.setPresentationLimits(
        Number(index), Number(minimum), Number(maximum)
    );
}

function enabled(value) {
    dialControl.presentation.enabled = Number(value) !== 0;
    dialControl.requestRedraw();
}

function active(value) {
    dialControl.presentation.active = Number(value) !== 0;
    dialControl.requestRedraw();
}

function scope(active, hasColor, red, green, blue, alpha) {
    dialControl.setScope(active, hasColor, red, green, blue, alpha);
}

const dialControl = new DialControl();
