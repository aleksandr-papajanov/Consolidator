autowatch = 1;
inlets = 1;
outlets = 1;

include("Project:/js/Presenters/Dial/DialPresentation.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var DialControlOptions = {
    maximumRingCount: 3,
    startAngle: Math.PI * 0.75,
    endAngle: Math.PI * 2.25,
    ringGap: 8,
    lineWidth: 3,
    indicatorWidth: 2,
    dragSensitivity: 0.007,
    background: [0.08, 0.08, 0.08, 1],
    ring: [0.55, 0.55, 0.55, 1],
    active: [0.95, 0.95, 0.95, 1],
    inactive: [0.35, 0.35, 0.35, 1],
    visualization: [0.35, 0.7, 1, 1]
};

function DialControl() {
    this.presentation = new DialPresentation();
    this.previewValues = [];
    this.dragging = false;
    this.dragIndex = 0;
    this.lastY = 0;
}

DialControl.prototype.applyPresentation = function (presentation) {
    if (!presentation) return;
    var previewIndex = this.dragging ? this.dragIndex : -1;
    var previewValue = previewIndex >= 0
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
    mgraphics.redraw();
};

DialControl.prototype.setPresentationValue = function (index, value) {
    var ring = this.presentation.rings[index];
    if (!ring) return;
    ring.value = this.clamp(Number(value), ring.minimum, ring.maximum);
    delete this.previewValues[index];
    mgraphics.redraw();
};

DialControl.prototype.setRingCount = function (count) {
    count = Math.max(0, Math.min(
        DialControlOptions.maximumRingCount,
        Math.floor(Number(count))
    ));
    if (!isFinite(count)) return;
    while (this.presentation.rings.length < count) {
        this.presentation.rings.push({
            value: 0,
            minimum: 0,
            maximum: 1,
            visualization: null,
            color: null
        });
    }
    this.presentation.rings.length = count;
    this.previewValues.length = count;
    mgraphics.redraw();
};

DialControl.prototype.setPresentationLimits = function (index, minimum, maximum) {
    var ring = this.presentation.rings[index];
    if (!ring) return;
    ring.minimum = Number(minimum);
    ring.maximum = Number(maximum);
    mgraphics.redraw();
};

DialControl.prototype.clamp = function (value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value));
};

DialControl.prototype.emit = function (name, payload) {
    if (payload === undefined) outlet(0, name);
    else if (payload instanceof Array) outlet(0, [name].concat(payload));
    else outlet(0, [name, payload]);
};

DialControl.prototype.setValue = function (index, value, output) {
    var ring = this.presentation.rings[index];
    if (!ring) return;
    var next = this.clamp(Number(value), ring.minimum, ring.maximum);
    if (!isFinite(next)) return;
    var currentValue = this.previewValues[index] === undefined
        ? ring.value : this.previewValues[index];
    if (Math.abs(next - currentValue) < 0.0000001) return;
    this.previewValues[index] = next;
    mgraphics.redraw();
    if (output) this.emit("valueChanged", [index, next]);
};

DialControl.prototype.resetValue = function (index) {
    var ring = this.presentation.rings[index];
    if (!ring) return;
    this.emit("reset", index);
};

DialControl.prototype.color = function (color, fallback) {
    return color && color.length >= 4 ? color : fallback;
};

DialControl.prototype.arc = function (centerX, centerY, radius, value, color, width) {
    var start = DialControlOptions.startAngle;
    var end = start + (DialControlOptions.endAngle - start) * value;
    mgraphics.set_source_rgba.apply(mgraphics, color);
    mgraphics.set_line_width(width);
    mgraphics.new_path();
    mgraphics.arc(centerX, centerY, radius, start, end);
    mgraphics.stroke();
};

DialControl.prototype.paintRing = function (
    ring, value, index, centerX, centerY, radius
) {
    var color = this.color(ring.color,
        this.presentation.enabled && this.presentation.active
        ? DialControlOptions.active : DialControlOptions.inactive);
    this.arc(centerX, centerY, radius, 1, DialControlOptions.ring,
        DialControlOptions.lineWidth);
    this.arc(centerX, centerY, radius, value, color,
        DialControlOptions.lineWidth);

    this.paintVisualization(ring.visualization, centerX, centerY, radius);
};

DialControl.prototype.paintVisualization = function (
    visualization, centerX, centerY, radius
) {
    if (!visualization) return;
    var value;
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
};

DialControl.prototype.paint = function () {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var centerX = width * 0.5;
    var centerY = height * 0.55;
    var radius = Math.max(1, Math.min(width, height) * 0.38);
    var rings = this.presentation.rings || [];

    mgraphics.set_source_rgba.apply(mgraphics, DialControlOptions.background);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    for (var index = 0; index < Math.min(
        rings.length, DialControlOptions.maximumRingCount
    ); index += 1) {
        var value = this.previewValues[index] === undefined
            ? rings[index].value : this.previewValues[index];
        this.paintRing(
            rings[index], value, index, centerX, centerY,
            radius - index * DialControlOptions.ringGap
        );
    }
};

DialControl.prototype.beginGesture = function (index, y) {
    if (!this.presentation.enabled || !this.presentation.active ||
            !this.presentation.rings[index]) return;
    this.dragging = true;
    this.dragIndex = index;
    this.lastY = y;
    this.emit("gestureBegan", index);
};

DialControl.prototype.drag = function (y) {
    if (!this.dragging) return;
    var ring = this.presentation.rings[this.dragIndex];
    var value = this.previewValues[this.dragIndex] === undefined
        ? ring.value : this.previewValues[this.dragIndex];
    this.setValue(
        this.dragIndex,
        value + (this.lastY - y) * DialControlOptions.dragSensitivity,
        true
    );
    this.lastY = y;
};

DialControl.prototype.endGesture = function () {
    if (!this.dragging) return;
    this.dragging = false;
    this.emit("gestureEnded", this.dragIndex);
};

DialControl.prototype.rejectTransaction = function () {
    this.dragging = false;
    this.previewValues = [];
    mgraphics.redraw();
};

function applyPresentation(presentation) {
    dialControl.applyPresentation(presentation);
}

function presentation(presentation) {
    applyPresentation(presentation);
}

function paint() {
    dialControl.paint();
}

function onclick(x, y, button, mod1, shift, caps, opt, mod2) {
    var index = dialControl.presentation.activeIndex || 0;
    dialControl.beginGesture(index, y);
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
    dialControl.resetValue(Number(index));
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
    mgraphics.redraw();
}

function active(value) {
    dialControl.presentation.active = Number(value) !== 0;
    mgraphics.redraw();
}

function activeIndex(value) {
    dialControl.presentation.activeIndex = Number(value);
    mgraphics.redraw();
}

function displayIndex(value) {
    dialControl.presentation.displayIndex = Number(value);
    mgraphics.redraw();
}

function ringCount(value) {
    dialControl.setRingCount(value);
}

var dialControl = new DialControl();
