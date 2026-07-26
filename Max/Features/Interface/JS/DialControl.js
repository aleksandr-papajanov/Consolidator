autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet: float/int, list <ring> <value>, count, enabled, enable, disable,
// outputValue, set, limits, indicator, clearIndicator, visualization, ringColor,
// clearRingColor, primaryValue, secondaryValue, tertiaryValue, and defaults.
// Outlet: list <ring> <normalizedValue>.
mgraphics.init();
include("JS/InterfaceVisualConfig.js");

var DialOptions = {
    maximumValueCount: 3,
    defaultValue: 0.5,
    emptySectionAngle: 120 * Math.PI / 180,
    emptySectionCenterAngle: Math.PI / 2,
    containerPaddingRatio: 0.05,
    arcBoundsHeightRatio: 1.72,
    ringGapRatio: 0.20,
    ringLineWidthDecay: 0.8,
    indicatorGap: 1.0,
    valuePaddingRatio: 0.56,
    dragSensitivity: 0.007
};

DialOptions.startAngle = DialOptions.emptySectionCenterAngle
    + DialOptions.emptySectionAngle * 0.5;
DialOptions.endAngle = DialOptions.emptySectionCenterAngle
    - DialOptions.emptySectionAngle * 0.5
    + Math.PI * 2.0;

declareattribute("valueCount", "getValueCount", "setValueCount", 1);
declareattribute("primaryValue", "getPrimaryValue", "setPrimaryValue", 1);
declareattribute("secondaryValue", "getSecondaryValue", "setSecondaryValue", 1);
declareattribute("tertiaryValue", "getTertiaryValue", "setTertiaryValue", 1);
declareattribute("primaryIndicator", "getPrimaryIndicator", "setPrimaryIndicator", 1);
declareattribute("secondaryIndicator", "getSecondaryIndicator", "setSecondaryIndicator", 1);
declareattribute("tertiaryIndicator", "getTertiaryIndicator", "setTertiaryIndicator", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);

function DialVisualization() {
    this.mode = "none";
    this.value = 0.0;
    this.relativeValue = 0.0;
}

function DialControl() {
    this.values = [0.5];
    this.visualizations = [new DialVisualization()];
    this.ringColors = [null];
    this.limits = [{ minimum: 0.0, maximum: 1.0 }];
    this.activeIndex = 0;
    this.displayIndex = 0;
    this.isDragging = false;
    this.enabled = true;
    this.lastY = 0;
    this.lastClickTime = 0;
    this.doubleClickInterval = 300;
    this.displayTask = new Task(this.ResetDisplayedValue, this);
}

DialControl.prototype.ResetDisplayedValue = function() {
    this.displayIndex = 0;
    mgraphics.redraw();
};

DialControl.prototype.ClampValue = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

DialControl.prototype.SetCount = function(count) {
    var nextCount = Math.max(
        1,
        Math.min(DialOptions.maximumValueCount, Math.floor(Number(count)))
    );
    var previousValues = this.values;
    var previousVisualizations = this.visualizations;
    var previousRingColors = this.ringColors;
    var previousLimits = this.limits;
    this.values = [];
    this.visualizations = [];
    this.ringColors = [];
    this.limits = [];
    for (var i = 0; i < nextCount; i++) {
        this.values.push(
            previousValues[i] === undefined
                ? DialOptions.defaultValue
                : previousValues[i]
        );
        this.visualizations.push(
            previousVisualizations[i] === undefined
                ? new DialVisualization()
                : previousVisualizations[i]
        );
        this.ringColors.push(previousRingColors[i] || null);
        this.limits.push(previousLimits[i] || { minimum: 0.0, maximum: 1.0 });
    }
    if (this.activeIndex >= nextCount) this.activeIndex = nextCount - 1;
    mgraphics.redraw();
};

DialControl.prototype.SetValue = function(index, value, shouldOutput) {
    if (index < 0 || index >= this.values.length) return;
    var nextValue = this.ClampValue(value);
    var limits = this.limits[index];
    nextValue = Math.max(limits.minimum, Math.min(limits.maximum, nextValue));
    if (nextValue === this.values[index] && !shouldOutput) return;
    this.values[index] = nextValue;
    if (shouldOutput) {
        this.displayIndex = index;
        this.displayTask.cancel();
        this.displayTask.schedule(1000);
    }
    mgraphics.redraw();
    if (!shouldOutput) return;
    if (this.values.length === 1) outlet(0, nextValue);
    else outlet(0, [index + 1, nextValue]);
};

DialControl.prototype.SetLimits = function(index, minimum, maximum) {
    var position = Number(index) - 1;
    var nextMinimum = this.ClampValue(minimum);
    var nextMaximum = this.ClampValue(maximum);
    if (position < 0 || position >= this.limits.length || nextMinimum > nextMaximum) return;
    this.limits[position] = { minimum: nextMinimum, maximum: nextMaximum };
    this.SetValue(position, this.values[position], false);
};

DialControl.prototype.GetRadius = function(index, width, height) {
    var padding = Math.min(width, height) * DialOptions.containerPaddingRatio;
    var baseRadius = Math.min(
        (width - padding * 2.0) * 0.5,
        (height - padding * 2.0) / DialOptions.arcBoundsHeightRatio
    );
    var ringGap = Math.max(
        InterfaceVisualConfig.controlLineWidth * 2.0,
        baseRadius * DialOptions.ringGapRatio
    );
    return Math.max(1.0, baseRadius - index * ringGap);
};

DialControl.prototype.GetCenterY = function(width, height) {
    var padding = Math.min(width, height) * DialOptions.containerPaddingRatio;
    return padding + this.GetRadius(0, width, height);
};

DialControl.prototype.GetRingLineWidth = function(index) {
    return Math.max(
        0.5,
        InterfaceVisualConfig.controlLineWidth
            * Math.pow(DialOptions.ringLineWidthDecay, index)
    );
};

DialControl.prototype.GetIndicatorRadius = function(index, width, height) {
    return Math.max(
        1.0,
        this.GetRadius(index, width, height)
            - this.GetRingLineWidth(index) * 0.5
            - InterfaceVisualConfig.indicatorLineWidth * 0.5
            - DialOptions.indicatorGap
    );
};

DialControl.prototype.GetRelativeVisualizationRadius = function(index, width, height) {
    return this.GetRadius(index, width, height)
        + this.GetRingLineWidth(index) * 0.5
        + InterfaceVisualConfig.indicatorLineWidth * 0.5
        + DialOptions.indicatorGap;
};

DialControl.prototype.PaintSignedVisualization = function(index, width, height) {
    var visualization = this.visualizations[index];
    if (!visualization || visualization.mode !== "signed"
        || visualization.value === 0) return;
    var value = visualization.value;
    var centerX = width * 0.5;
    var centerY = this.GetCenterY(width, height);
    var centerAngle = (DialOptions.startAngle + DialOptions.endAngle) * 0.5;
    var maximumSweep = (DialOptions.endAngle - DialOptions.startAngle) * 0.5;
    var valueAngle = centerAngle + maximumSweep * value;
    var startAngle = value < 0 ? valueAngle : centerAngle;
    var endAngle = value < 0 ? centerAngle : valueAngle;

    mgraphics.set_line_width(InterfaceVisualConfig.indicatorLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(InterfaceVisualConfig.indicatorColor);
    mgraphics.new_path();
    mgraphics.arc(
        centerX,
        centerY,
        this.GetIndicatorRadius(index, width, height),
        startAngle,
        endAngle
    );
    mgraphics.stroke();
};

DialControl.prototype.PaintRelativeVisualization = function(index, width, height) {
    var visualization = this.visualizations[index];
    if (!visualization || visualization.relativeValue === 0) return;

    var sweep = DialOptions.endAngle - DialOptions.startAngle;
    var anchorAngle = DialOptions.startAngle + sweep * this.values[index];
    var relativeAngle = anchorAngle + sweep * visualization.relativeValue;
    relativeAngle = Math.max(
        DialOptions.startAngle,
        Math.min(DialOptions.endAngle, relativeAngle)
    );
    var startAngle = Math.min(anchorAngle, relativeAngle);
    var endAngle = Math.max(anchorAngle, relativeAngle);

    mgraphics.set_line_width(InterfaceVisualConfig.indicatorLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(InterfaceVisualConfig.reductionColor);
    mgraphics.new_path();
    mgraphics.arc(
        width * 0.5,
        this.GetCenterY(width, height),
        this.GetRelativeVisualizationRadius(index, width, height),
        startAngle,
        endAngle
    );
    mgraphics.stroke();
};

DialControl.prototype.BlendColor = function(left, right, amount) {
    var value = this.ClampValue(amount);
    return [
        left[0] + (right[0] - left[0]) * value,
        left[1] + (right[1] - left[1]) * value,
        left[2] + (right[2] - left[2]) * value,
        left[3] + (right[3] - left[3]) * value
    ];
};

DialControl.prototype.GetRingValueColor = function(index) {
    if (this.ringColors[index]) return this.ringColors[index];
    var visualization = this.visualizations[index];
    if (!visualization || visualization.mode !== "color") {
        return InterfaceVisualConfig.valueColor;
    }
    return this.BlendColor(
        InterfaceVisualConfig.valueColor,
        InterfaceVisualConfig.alertColor,
        visualization.value
    );
};

DialControl.prototype.SetRingColor = function(index, red, green, blue, alpha) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.ringColors.length) return;
    this.ringColors[position] = [
        Number(red), Number(green), Number(blue),
        alpha === undefined ? 1 : Number(alpha)
    ];
    mgraphics.redraw();
};

DialControl.prototype.ClearRingColor = function(index) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.ringColors.length) return;
    this.ringColors[position] = null;
    mgraphics.redraw();
};

DialControl.prototype.PaintRing = function(index, width, height) {
    var centerX = width * 0.5;
    var centerY = this.GetCenterY(width, height);
    var radius = this.GetRadius(index, width, height);
    var angle = DialOptions.startAngle
        + (DialOptions.endAngle - DialOptions.startAngle) * this.values[index];

    mgraphics.set_line_width(this.GetRingLineWidth(index));
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(InterfaceVisualConfig.trackColor);
    mgraphics.new_path();
    mgraphics.arc(centerX, centerY, radius, DialOptions.startAngle, DialOptions.endAngle);
    mgraphics.stroke();

    if (this.enabled) mgraphics.set_source_rgba(this.GetRingValueColor(index));
    else mgraphics.set_source_rgba(InterfaceVisualConfig.trackColor);
    mgraphics.new_path();
    mgraphics.arc(centerX, centerY, radius, DialOptions.startAngle, angle);
    mgraphics.stroke();
    this.PaintRelativeVisualization(index, width, height);
    this.PaintSignedVisualization(index, width, height);
};

DialControl.prototype.Paint = function() {
    var size = mgraphics.size;
    var width = size[0];
    var height = size[1];
    for (var i = 0; i < this.values.length; i++) {
        this.PaintRing(i, width, height);
    }
    this.PaintValues(width, height);
};

DialControl.prototype.PaintValues = function(width, height) {
    var innerRadius = this.GetRadius(this.values.length - 1, width, height);
    var availableDiameter = innerRadius * 2.0
        * (1.0 - DialOptions.valuePaddingRatio);
    var maxFontSize = availableDiameter / 1.15;
    var fontSize = Math.max(InterfaceVisualConfig.minimumTextFontSize, Math.min(
        Math.min(width, height) * InterfaceVisualConfig.dialValueFontSizeRatio,
        maxFontSize
    ));
    var lineHeight = fontSize * 1.15;
    var centerY = this.GetCenterY(width, height);
    var text = this.values[this.displayIndex].toFixed(2);
    var textSize;

    mgraphics.select_font_face("Arial");
    mgraphics.set_font_size(fontSize);
    mgraphics.set_source_rgba(InterfaceVisualConfig.textColor);
    textSize = mgraphics.text_measure(text);
    mgraphics.move_to((width - textSize[0]) * 0.5, centerY + lineHeight * 0.35);
    mgraphics.show_text(text);
};

DialControl.prototype.HandleDrag = function(y) {
    var delta = this.lastY - y;
    this.lastY = y;
    this.SetValue(
        this.activeIndex,
        this.values[this.activeIndex] + delta * DialOptions.dragSensitivity,
        true
    );
};

DialControl.prototype.SetActiveIndex = function(opt, mod2) {
    var requestedIndex = opt ? 2 : (mod2 ? 1 : 0);
    this.activeIndex = Math.min(requestedIndex, this.values.length - 1);
};

DialControl.prototype.HandleClick = function(opt, mod2) {
    if (!this.enabled) return;
    var currentTime = new Date().getTime();
    this.SetActiveIndex(opt, mod2);
    if (currentTime - this.lastClickTime <= this.doubleClickInterval) {
        this.lastClickTime = 0;
        this.isDragging = false;
        this.SetValue(this.activeIndex, DialOptions.defaultValue, true);
        return;
    }
    this.lastClickTime = currentTime;
    this.displayIndex = this.activeIndex;
    this.isDragging = true;
    mgraphics.redraw();
};

DialControl.prototype.BeginDrag = function(y) {
    this.lastY = y;
};

DialControl.prototype.HandleDragEvent = function(x, y, button, opt, mod2) {
    if (!this.enabled) return;
    this.SetActiveIndex(opt, mod2);
    if (this.isDragging) this.HandleDrag(y);
};

DialControl.prototype.HandleRelease = function(x, y, button, modifiers, inTime, outTime) {
    this.isDragging = false;
};

DialControl.prototype.HandleMessage = function(value) {
    this.SetValue(this.activeIndex, value, false);
};

DialControl.prototype.SetEnabled = function(value) {
    this.enabled = Number(value) !== 0;
    if (!this.enabled) this.isDragging = false;
    mgraphics.redraw();
};

DialControl.prototype.SetIndexedValue = function(index, value) {
    this.SetValue(Number(index) - 1, value, false);
};

DialControl.prototype.SetVisualization = function(index, mode, value) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.visualizations.length) return;
    var normalizedMode = String(mode);
    if (normalizedMode !== "none"
        && normalizedMode !== "signed"
        && normalizedMode !== "color"
        && normalizedMode !== "relative") return;
    var numericValue = Number(value);
    if (!isFinite(numericValue)) return;
    var visualization = this.visualizations[position];
    if (normalizedMode === "relative") {
        visualization.relativeValue = Math.max(-1.0, Math.min(1.0, numericValue));
        mgraphics.redraw();
        return;
    }
    if (normalizedMode === "none") visualization.relativeValue = 0.0;
    visualization.mode = normalizedMode;
    visualization.value = normalizedMode === "signed"
        ? Math.max(-1.0, Math.min(1.0, numericValue))
        : this.ClampValue(numericValue);
    mgraphics.redraw();
};

DialControl.prototype.SetIndicator = function(index, value) {
    this.SetVisualization(index, "signed", value);
};

DialControl.prototype.ClearIndicator = function(index) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.visualizations.length) return;
    this.visualizations[position].mode = "none";
    this.visualizations[position].value = 0.0;
    this.visualizations[position].relativeValue = 0.0;
    mgraphics.redraw();
};

DialControl.prototype.OutputValue = function() {
    if (this.values.length === 1) {
        outlet(0, this.values[0]);
        return;
    }
    for (var i = 0; i < this.values.length; i++) {
        outlet(0, [i + 1, this.values[i]]);
    }
};

var dialControl = new DialControl();

function getValueCount() {
    return dialControl.values.length;
}

function setValueCount(value) {
    dialControl.SetCount(value);
}

function getPrimaryValue() {
    return dialControl.values[0];
}

function getSecondaryValue() {
    return dialControl.values.length > 1 ? dialControl.values[1] : 0.0;
}

function getTertiaryValue() {
    return dialControl.values.length > 2 ? dialControl.values[2] : 0.0;
}

function setPrimaryValue(value) {
    dialControl.SetIndexedValue(1, value);
}

function setSecondaryValue(value) {
    dialControl.SetIndexedValue(2, value);
}

function setTertiaryValue(value) {
    dialControl.SetIndexedValue(3, value);
}

function getPrimaryIndicator() {
    return dialControl.visualizations[0].value;
}

function getSecondaryIndicator() {
    return dialControl.visualizations.length > 1
        ? dialControl.visualizations[1].value
        : 0.0;
}

function getTertiaryIndicator() {
    return dialControl.visualizations.length > 2
        ? dialControl.visualizations[2].value
        : 0.0;
}

function setPrimaryIndicator(value) {
    dialControl.SetIndicator(1, value);
}

function setSecondaryIndicator(value) {
    dialControl.SetIndicator(2, value);
}

function setTertiaryIndicator(value) {
    dialControl.SetIndicator(3, value);
}

function getEnabled() {
    return dialControl.enabled ? 1 : 0;
}

function setEnabled(value) {
    dialControl.SetEnabled(value);
}

function paint() {
    dialControl.Paint();
}

function msg_float(value) {
    dialControl.HandleMessage(value);
}

function msg_int(value) {
    dialControl.HandleMessage(value);
}

function count(value) {
    dialControl.SetCount(value);
}

function enabled(value) {
    dialControl.SetEnabled(value);
}

function enable() {
    dialControl.SetEnabled(1);
}

function disable() {
    dialControl.SetEnabled(0);
}

function outputValue() {
    dialControl.OutputValue();
}

function set(index, value) {
    dialControl.SetIndexedValue(index, value);
}

function limits(index, minimum, maximum) {
    dialControl.SetLimits(index, minimum, maximum);
}

function indicator(index, value) {
    dialControl.SetIndicator(index, value);
}

function clearIndicator(index) {
    dialControl.ClearIndicator(index);
}

function visualization(index, mode, value) {
    dialControl.SetVisualization(index, mode, value);
}

function ringColor(index, red, green, blue, alpha) {
    dialControl.SetRingColor(index, red, green, blue, alpha);
}

function clearRingColor(index) {
    dialControl.ClearRingColor(index);
}

function list() {
    var values = arrayfromargs(arguments);
    if (values.length === 2) dialControl.SetIndexedValue(values[0], values[1]);
    else if (values.length > 0) dialControl.HandleMessage(values[0]);
}

function onclick(x, y, button, mod1, shift, caps, opt, mod2) {
    dialControl.HandleClick(opt, mod2);
    dialControl.BeginDrag(y);
}

function ondrag(x, y, button, mod1, shift, caps, opt, mod2) {
    if (button === 0) {
        dialControl.HandleRelease(x, y, button, mod1, shift, caps, opt, mod2);
        return;
    }
    dialControl.HandleDragEvent(x, y, button, opt, mod2);
}

function onidleout(x, y, button, modifiers, inTime, outTime) {
    dialControl.HandleRelease(x, y, button, modifiers, inTime, outTime);
}

function onresize(width, height) {
    mgraphics.redraw();
}
