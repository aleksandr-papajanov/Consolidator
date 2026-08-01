autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet: float/int, list <ring> <value>, count, enabled, enable, disable,
// outputValue, set, limits, indicator, levelVisualization, reductionVisualization,
// clearIndicator, visualization, ringColor,
// clearRingColor, displayRange <ring> <min> <max> <log> <decimals> <suffix>,
// step <ring> <normalized-step>, primaryValue, secondaryValue, tertiaryValue,
// active, activityEnabled, listenEnabled, levelMatchEnabled <0|1>,
// onsetMatchEnabled <0|1>, autoMatchEnabled <0|1>, levelMatch <0|1>,
// onsetMatch <0|1>, autoMatch <0|1>, listen, and defaults.
// Outlet: list <ring> <normalizedValue>, active <0|1>, listen <0|1>,
// levelMatch <0|1>, onsetMatch <0|1>, or autoMatch <0|1>.
mgraphics.init();
include("../../Configuration/InterfaceTheme.js");
include("DialOptions.js");
include("../Core/ControlState.js");
include("../Core/ControlLayout.js");
include("DialViewModel.js");
include("DialRenderer.js");

declareattribute("valueCount", "getValueCount", "setValueCount", 1);
declareattribute("primaryValue", "getPrimaryValue", "setPrimaryValue", 1);
declareattribute("secondaryValue", "getSecondaryValue", "setSecondaryValue", 1);
declareattribute("tertiaryValue", "getTertiaryValue", "setTertiaryValue", 1);
declareattribute("primaryIndicator", "getPrimaryIndicator", "setPrimaryIndicator", 1);
declareattribute("secondaryIndicator", "getSecondaryIndicator", "setSecondaryIndicator", 1);
declareattribute("tertiaryIndicator", "getTertiaryIndicator", "setTertiaryIndicator", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);
declareattribute("activityEnabled", "getActivityEnabled", "setActivityEnabled", 1);
declareattribute("listenEnabled", "getListenEnabled", "setListenEnabled", 1);
declareattribute("levelMatchEnabled", "getLevelMatchEnabled", "setLevelMatchEnabled", 1);
declareattribute("onsetMatchEnabled", "getOnsetMatchEnabled", "setOnsetMatchEnabled", 1);
declareattribute("autoMatchEnabled", "getAutoMatchEnabled", "setAutoMatchEnabled", 1);

function DialVisualization() {
    this.mode = "none";
    this.value = 0.0;
    this.relativeValue = 0.0;
    this.peakValue = 0.0;
    this.smoothedValue = 0.0;
    this.reductionValue = 0.0;
}

function DialControl() {
    this.defaultValues = [DialOptions.defaultValue];
    this.values = [0.5];
    this.rawValues = [0.5];
    this.visualizations = [new DialVisualization()];
    this.ringColors = [null];
    this.limits = [{ minimum: 0.0, maximum: 1.0 }];
    this.steps = [0.0];
    this.displayRanges = [null];
    this.activeIndex = 0;
    this.displayIndex = 0;
    this.active = true;
    this.activityEnabled = false;
    this.listenEnabled = false;
    this.levelMatchEnabled = false;
    this.onsetMatchEnabled = false;
    this.autoMatchEnabled = false;
    this.levelMatchActive = false;
    this.onsetMatchActive = false;
    this.autoMatchActive = false;
    this.listen = false;
    this.isDragging = false;
    this.enabled = true;
    this.lastY = 0;
    this.lastClickTime = 0;
    this.doubleClickInterval = 300;
    this.displayTask = new Task(this.ResetDisplayedValue, this);
    this.viewModel = new DialViewModel();
    this.viewState = this.viewModel;
    this.renderer = new DialRenderer();
    this.layout = new ControlLayout();
}

DialControl.prototype.ResetDisplayedValue = function() {
    this.displayIndex = 0;
    mgraphics.redraw();
};

DialControl.prototype.ScheduleDisplayedReset = function() {
    this.displayTask.cancel();
    this.displayTask.schedule(1000);
};

DialControl.prototype.Dispose = function() {
    this.displayTask.cancel();
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
    var previousRawValues = this.rawValues;
    var previousVisualizations = this.visualizations;
    var previousRingColors = this.ringColors;
    var previousLimits = this.limits;
    var previousSteps = this.steps;
    var previousDisplayRanges = this.displayRanges;
    var previousDefaults = this.defaultValues;
    this.values = [];
    this.rawValues = [];
    this.visualizations = [];
    this.ringColors = [];
    this.limits = [];
    this.steps = [];
    this.displayRanges = [];
    for (var i = 0; i < nextCount; i++) {
        this.values.push(
            previousValues[i] === undefined
                ? DialOptions.defaultValue
                : previousValues[i]
        );
        this.rawValues.push(
            previousRawValues[i] === undefined
                ? this.values[i]
                : previousRawValues[i]
        );
        this.visualizations.push(
            previousVisualizations[i] === undefined
                ? new DialVisualization()
                : previousVisualizations[i]
        );
        this.ringColors.push(previousRingColors[i] || null);
        this.limits.push(previousLimits[i] || { minimum: 0.0, maximum: 1.0 });
        this.steps.push(previousSteps[i] === undefined ? 0.0 : previousSteps[i]);
        this.displayRanges.push(previousDisplayRanges[i] || null);
        this.defaultValues[i] = previousDefaults[i] === undefined
            ? DialOptions.defaultValue
            : previousDefaults[i];
    }
    if (this.activeIndex >= nextCount) this.activeIndex = nextCount - 1;
    mgraphics.redraw();
};

DialControl.prototype.SetValue = function(index, value, shouldOutput) {
    if (index < 0 || index >= this.values.length) return;
    var rawValue = this.ClampValue(value);
    var limits = this.limits[index];
    rawValue = Math.max(limits.minimum, Math.min(limits.maximum, rawValue));
    this.rawValues[index] = rawValue;
    var nextValue = rawValue;
    nextValue = this.SnapValue(index, nextValue);
    nextValue = Math.max(limits.minimum, Math.min(limits.maximum, nextValue));
    if (Math.abs(nextValue - this.values[index]) < 0.0000001) return;
    this.values[index] = nextValue;
    if (shouldOutput) {
        this.displayIndex = index;
        if (!this.isDragging) this.ScheduleDisplayedReset();
    }
    mgraphics.redraw();
    if (!shouldOutput) return;
    if (this.values.length === 1) outlet(0, nextValue);
    else outlet(0, [index + 1, nextValue]);
};

DialControl.prototype.SnapValue = function(index, value) {
    var step = Number(this.steps[index]);
    if (!isFinite(step) || step <= 0) return value;
    return Math.round(value / step) * step;
};

DialControl.prototype.SetStep = function(index, step) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.steps.length) return;
    var numericStep = Number(step);
    this.steps[position] = isFinite(numericStep) && numericStep > 0
        ? numericStep
        : 0.0;
    this.SetValue(position, this.values[position], false);
};

DialControl.prototype.SetLimits = function(index, minimum, maximum) {
    var position = Number(index) - 1;
    var nextMinimum = this.ClampValue(minimum);
    var nextMaximum = this.ClampValue(maximum);
    if (position < 0 || position >= this.limits.length || nextMinimum > nextMaximum) return;
    this.limits[position] = { minimum: nextMinimum, maximum: nextMaximum };
    this.SetValue(position, this.values[position], false);
};

DialControl.prototype.SetDisplayRange = function(
    index,
    minimum,
    maximum,
    logarithmic,
    decimals,
    suffix
) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.displayRanges.length) return;
    this.displayRanges[position] = {
        minimum: Number(minimum),
        maximum: Number(maximum),
        logarithmic: Number(logarithmic) !== 0,
        decimals: Math.max(0, Math.floor(Number(decimals))),
        suffix: suffix === undefined ? "" : String(suffix)
    };
    this.SetValue(position, this.values[position], false);
    mgraphics.redraw();
};

DialControl.prototype.SetDefaultValue = function(index, value) {
    if (index < 0) return;
    while (this.defaultValues.length <= index) {
        this.defaultValues.push(DialOptions.defaultValue);
    }
    this.defaultValues[index] = Math.max(0.0, Math.min(1.0, Number(value)));
};

DialControl.prototype.ResetValue = function(index, shouldOutput) {
    if (index < 0 || index >= this.values.length) return;
    this.SetValue(
        index,
        this.defaultValues[index] !== undefined
            ? this.defaultValues[index]
            : DialOptions.defaultValue,
        shouldOutput
    );
};

DialControl.prototype.ResetActiveValue = function() {
    this.ResetValue(this.activeIndex, true);
};

DialControl.prototype.FormatValue = function(index) {
    var normalized = this.values[index];
    var range = this.displayRanges[index];
    if (!range) return normalized.toFixed(2);
    var absolute = range.logarithmic && range.minimum > 0
        ? range.minimum * Math.pow(range.maximum / range.minimum, normalized)
        : range.minimum + normalized * (range.maximum - range.minimum);
    return absolute.toFixed(range.decimals) + range.suffix;
};

DialControl.prototype.GetRadius = function(index, width, height) {
    var padding = InterfaceTheme.geometry.controlLineWidth;
    var baseRadius = Math.min(
        (width - padding * 2.0) * 0.5,
        (height - padding * 2.0) / DialOptions.arcBoundsHeightRatio
    );
    var ringGap = Math.max(
        InterfaceTheme.geometry.controlLineWidth * 2.0,
        baseRadius * DialOptions.ringGapRatio
    );
    return Math.max(1.0, baseRadius - index * ringGap);
};

DialControl.prototype.GetCenterY = function(width, height) {
    var padding = InterfaceTheme.geometry.controlLineWidth;
    return padding + this.GetRadius(0, width, height);
};

DialControl.prototype.GetRingLineWidth = function(index) {
    return Math.max(
        0.5,
        InterfaceTheme.geometry.controlLineWidth
            * Math.pow(DialOptions.ringLineWidthDecay, index)
    );
};

DialControl.prototype.GetIndicatorRadius = function(index, width, height) {
    return Math.max(
        1.0,
        this.GetRadius(index, width, height)
            - this.GetRingLineWidth(index) * 0.5
            - InterfaceTheme.geometry.indicatorLineWidth * 0.5
            - DialOptions.indicatorGap
    );
};

DialControl.prototype.GetRelativeVisualizationRadius = function(index, width, height) {
    return this.GetRadius(index, width, height)
        + this.GetRingLineWidth(index) * 0.5
        + InterfaceTheme.geometry.indicatorLineWidth * 0.5
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

    mgraphics.set_line_width(InterfaceTheme.geometry.indicatorLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(InterfaceTheme.colors.primaryAccent);
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

    this.PaintRelativeArc(
        index,
        width,
        height,
        visualization.relativeValue,
        InterfaceTheme.colors.tertiaryAccent
    );
};

DialControl.prototype.PaintRelativeArc = function(
    index,
    width,
    height,
    value,
    color
) {
    if (value === 0) return;

    var sweep = DialOptions.endAngle - DialOptions.startAngle;
    var anchorAngle = DialOptions.startAngle + sweep * this.values[index];
    var relativeAngle = anchorAngle + sweep * value;
    relativeAngle = Math.max(
        DialOptions.startAngle,
        Math.min(DialOptions.endAngle, relativeAngle)
    );
    var startAngle = Math.min(anchorAngle, relativeAngle);
    var endAngle = Math.max(anchorAngle, relativeAngle);

    mgraphics.set_line_width(InterfaceTheme.geometry.indicatorLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(color[0], color[1], color[2], color[3]);
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

DialControl.prototype.PaintReductionVisualization = function(index, width, height) {
    var visualization = this.visualizations[index];
    if (!visualization) return;
    this.PaintRelativeArc(
        index,
        width,
        height,
        -visualization.reductionValue,
        InterfaceTheme.colors.tertiaryAccent
    );
};

DialControl.prototype.PaintLevelArc = function(
    index,
    width,
    height,
    value,
    color
) {
    if (value === 0) return;
    var centerAngle = (DialOptions.startAngle + DialOptions.endAngle) * 0.5;
    var maximumSweep = (DialOptions.endAngle - DialOptions.startAngle) * 0.5;
    var valueAngle = centerAngle + maximumSweep * value;
    var startAngle = value < 0 ? valueAngle : centerAngle;
    var endAngle = value < 0 ? centerAngle : valueAngle;
    mgraphics.set_line_width(InterfaceTheme.geometry.indicatorLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(color[0], color[1], color[2], color[3]);
    mgraphics.new_path();
    mgraphics.arc(
        width * 0.5,
        this.GetCenterY(width, height),
        this.GetIndicatorRadius(index, width, height),
        startAngle,
        endAngle
    );
    mgraphics.stroke();
};

DialControl.prototype.PaintLevelVisualization = function(index, width, height) {
    var visualization = this.visualizations[index];
    if (!visualization || visualization.mode !== "level") return;
    this.PaintLevelArc(
        index,
        width,
        height,
        visualization.peakValue,
        InterfaceTheme.colors.tertiaryAccentVariant
    );
    this.PaintLevelArc(
        index,
        width,
        height,
        visualization.smoothedValue,
        InterfaceTheme.colors.tertiaryAccent
    );
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
        return InterfaceTheme.colors.primaryAccent;
    }
    return this.BlendColor(
        InterfaceTheme.colors.primaryAccent,
        InterfaceTheme.colors.tertiaryAccent,
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
    var sweep = DialOptions.endAngle - DialOptions.startAngle;
    var limits = this.limits[index];
    var minimumAngle = DialOptions.startAngle + sweep * limits.minimum;
    var maximumAngle = DialOptions.startAngle + sweep * limits.maximum;
    var angle = DialOptions.startAngle + sweep * this.values[index];

    mgraphics.set_line_width(this.GetRingLineWidth(index));
    mgraphics.set_line_cap("round");
    mgraphics.set_source_rgba(InterfaceTheme.colors.trackLimited);
    mgraphics.new_path();
    mgraphics.arc(centerX, centerY, radius, DialOptions.startAngle, DialOptions.endAngle);
    mgraphics.stroke();

    mgraphics.set_source_rgba(InterfaceTheme.colors.track);
    mgraphics.new_path();
    mgraphics.arc(centerX, centerY, radius, minimumAngle, maximumAngle);
    mgraphics.stroke();

    if (this.enabled && this.active) mgraphics.set_source_rgba(this.GetRingValueColor(index));
    else mgraphics.set_source_rgba(InterfaceTheme.colors.track);
    mgraphics.new_path();
    mgraphics.arc(centerX, centerY, radius, DialOptions.startAngle, angle);
    mgraphics.stroke();
    this.PaintRelativeVisualization(index, width, height);
    this.PaintSignedVisualization(index, width, height);
    this.PaintLevelVisualization(index, width, height);
    this.PaintReductionVisualization(index, width, height);
};

DialControl.prototype.PaintRings = function() {
    var size = this.layout.Size();
    var width = size.width;
    var height = size.height;
    this.viewState.enabled = this.enabled;
    this.viewState.value = this.values[this.displayIndex];
    for (var i = 0; i < this.values.length; i++) {
        this.PaintRing(i, width, height);
    }
    this.PaintValues(width, height);
    this.PaintActivityButton(width, height);
};

DialControl.prototype.Paint = function() {
    this.renderer.Paint(this);
};

DialControl.prototype.PaintValues = function(width, height) {
    var fontSize = InterfaceTheme.typography.minimumSize;
    var centerY = this.GetCenterY(width, height);
    var text = this.FormatValue(this.displayIndex);
    var textSize;

    mgraphics.select_font_face(InterfaceTheme.typography.fontFamily);
    mgraphics.set_font_size(fontSize);
    mgraphics.set_source_rgba(this.enabled && this.active
        ? InterfaceTheme.colors.text
        : InterfaceTheme.colors.textInactive);
    textSize = mgraphics.text_measure(text);
    var fontExtents = mgraphics.font_extents();
    mgraphics.move_to(
        (width - textSize[0]) * 0.5,
        centerY + (fontExtents[0] - fontExtents[1]) * 0.5
    );
    mgraphics.show_text(text);
};

DialControl.prototype.GetActivityButtonRadius = function(width, height) {
    return Math.max(2.0, Math.min(width, height)
        * DialOptions.activityButtonRadiusRatio);
};

DialControl.prototype.GetActionButtonCount = function() {
    return (this.activityEnabled ? 1 : 0)
        + (this.levelMatchEnabled ? 1 : 0)
        + (this.onsetMatchEnabled ? 1 : 0)
        + (this.autoMatchEnabled ? 1 : 0)
        + (this.listenEnabled ? 1 : 0);
};

DialControl.prototype.GetActionButtonCenter = function(position, width, height) {
    var radius = this.GetActivityButtonRadius(width, height);
    var count = this.GetActionButtonCount();
    var gap = DialOptions.activityButtonGap;
    var groupWidth = count * radius * 2.0 + Math.max(0, count - 1) * gap;
    var centerX = (width - groupWidth) * 0.5 + radius
        + position * (radius * 2.0 + gap);
    return [
        centerX,
        Math.max(radius + 1.0, height - radius - InterfaceTheme.geometry.controlLineWidth)
    ];
};

DialControl.prototype.GetActivityButtonCenter = function(width, height) {
    return this.GetActionButtonCenter(0, width, height);
};

DialControl.prototype.GetLevelMatchButtonCenter = function(width, height) {
    return this.GetActionButtonCenter(this.activityEnabled ? 1 : 0, width, height);
};

DialControl.prototype.GetListenButtonCenter = function(width, height) {
    return this.GetActionButtonCenter(
        (this.activityEnabled ? 1 : 0)
            + (this.levelMatchEnabled ? 1 : 0)
            + (this.onsetMatchEnabled ? 1 : 0)
            + (this.autoMatchEnabled ? 1 : 0),
        width,
        height
    );
};

DialControl.prototype.GetOnsetMatchButtonCenter = function(width, height) {
    return this.GetActionButtonCenter(
        (this.activityEnabled ? 1 : 0) + (this.levelMatchEnabled ? 1 : 0),
        width,
        height
    );
};

DialControl.prototype.GetAutoMatchButtonCenter = function(width, height) {
    return this.GetActionButtonCenter(
        (this.activityEnabled ? 1 : 0)
            + (this.levelMatchEnabled ? 1 : 0)
            + (this.onsetMatchEnabled ? 1 : 0),
        width,
        height
    );
};

DialControl.prototype.IsActivityButtonPoint = function(x, y, width, height) {
    if (!this.activityEnabled) return false;
    var center = this.GetActivityButtonCenter(width, height);
    var radius = this.GetActivityButtonRadius(width, height) + 2.0;
    var dx = Number(x) - center[0];
    var dy = Number(y) - center[1];
    return dx * dx + dy * dy <= radius * radius;
};

DialControl.prototype.IsListenButtonPoint = function(x, y, width, height) {
    if (!this.listenEnabled) return false;
    var center = this.GetListenButtonCenter(width, height);
    var radius = this.GetActivityButtonRadius(width, height) + 2.0;
    var dx = Number(x) - center[0];
    var dy = Number(y) - center[1];
    return dx * dx + dy * dy <= radius * radius;
};

DialControl.prototype.IsLevelMatchButtonPoint = function(x, y, width, height) {
    if (!this.levelMatchEnabled) return false;
    var center = this.GetLevelMatchButtonCenter(width, height);
    var radius = this.GetActivityButtonRadius(width, height) + 2.0;
    var dx = Number(x) - center[0];
    var dy = Number(y) - center[1];
    return dx * dx + dy * dy <= radius * radius;
};

DialControl.prototype.IsOnsetMatchButtonPoint = function(x, y, width, height) {
    if (!this.onsetMatchEnabled) return false;
    var center = this.GetOnsetMatchButtonCenter(width, height);
    var radius = this.GetActivityButtonRadius(width, height) + 2.0;
    var dx = Number(x) - center[0];
    var dy = Number(y) - center[1];
    return dx * dx + dy * dy <= radius * radius;
};

DialControl.prototype.IsAutoMatchButtonPoint = function(x, y, width, height) {
    if (!this.autoMatchEnabled) return false;
    var center = this.GetAutoMatchButtonCenter(width, height);
    var radius = this.GetActivityButtonRadius(width, height) + 2.0;
    var dx = Number(x) - center[0];
    var dy = Number(y) - center[1];
    return dx * dx + dy * dy <= radius * radius;
};

DialControl.prototype.PaintActionButton = function(center, radius, enabled, active, color) {
    var buttonColor = !enabled ? InterfaceTheme.colors.textDisabled
        : active ? color : InterfaceTheme.colors.textInactive;
    mgraphics.set_line_width(DialOptions.activityButtonLineWidth);
    mgraphics.set_source_rgba(buttonColor);
    mgraphics.new_path();
    mgraphics.arc(center[0], center[1], radius, 0, Math.PI * 2.0);
    if (active && enabled) mgraphics.fill();
    else mgraphics.stroke();
    if (active && enabled) {
        mgraphics.set_source_rgba(InterfaceTheme.colors.background);
        mgraphics.new_path();
        mgraphics.arc(center[0], center[1], radius * 0.38, 0, Math.PI * 2.0);
        mgraphics.fill();
    }
};

DialControl.prototype.GetActionButtonColor = function() {
    return this.ringColors[0]
        ? this.ringColors[0]
        : InterfaceTheme.colors.secondaryAccent;
};

DialControl.prototype.PaintLevelMatchButton = function(center, radius) {
    var color = this.enabled
        ? this.GetActionButtonColor()
        : InterfaceTheme.colors.textDisabled;
    mgraphics.set_line_width(DialOptions.activityButtonLineWidth);
    mgraphics.set_source_rgba(color);
    mgraphics.new_path();
    mgraphics.arc(center[0], center[1], radius, 0, Math.PI * 2.0);
    if (this.levelMatchActive && this.enabled) mgraphics.fill();
    else mgraphics.stroke();
    mgraphics.set_source_rgba(this.levelMatchActive && this.enabled
        ? InterfaceTheme.colors.background
        : color);
    mgraphics.new_path();
    mgraphics.move_to(center[0] - radius * 0.38, center[1] - radius * 0.18);
    mgraphics.line_to(center[0] + radius * 0.38, center[1] - radius * 0.18);
    mgraphics.move_to(center[0] - radius * 0.38, center[1] + radius * 0.18);
    mgraphics.line_to(center[0] + radius * 0.38, center[1] + radius * 0.18);
    mgraphics.stroke();
};

DialControl.prototype.PaintOnsetMatchButton = function(center, radius) {
    var color = this.enabled
        ? this.GetActionButtonColor()
        : InterfaceTheme.colors.textDisabled;
    mgraphics.set_line_width(DialOptions.activityButtonLineWidth);
    mgraphics.set_source_rgba(color);
    mgraphics.new_path();
    mgraphics.arc(center[0], center[1], radius, 0, Math.PI * 2.0);
    if (this.onsetMatchActive && this.enabled) mgraphics.fill();
    else mgraphics.stroke();
    mgraphics.set_source_rgba(this.onsetMatchActive && this.enabled
        ? InterfaceTheme.colors.background
        : color);
    mgraphics.new_path();
    mgraphics.move_to(center[0] - radius * 0.35, center[1] + radius * 0.25);
    mgraphics.line_to(center[0] - radius * 0.08, center[1] - radius * 0.25);
    mgraphics.line_to(center[0] + radius * 0.35, center[1] + radius * 0.25);
    mgraphics.stroke();
};

DialControl.prototype.PaintAutoMatchButton = function(center, radius) {
    var color = this.enabled
        ? InterfaceTheme.colors.secondaryAccent
        : InterfaceTheme.colors.textDisabled;
    mgraphics.set_line_width(DialOptions.activityButtonLineWidth);
    mgraphics.set_source_rgba(color);
    mgraphics.new_path();
    mgraphics.arc(center[0], center[1], radius, 0, Math.PI * 2.0);
    if (this.autoMatchActive && this.enabled) mgraphics.fill();
    else mgraphics.stroke();
    mgraphics.set_source_rgba(this.autoMatchActive && this.enabled
        ? InterfaceTheme.colors.background
        : color);
    mgraphics.new_path();
    mgraphics.move_to(center[0] - radius * 0.35, center[1] + radius * 0.25);
    mgraphics.line_to(center[0] - radius * 0.08, center[1] - radius * 0.25);
    mgraphics.line_to(center[0] + radius * 0.35, center[1] + radius * 0.25);
    mgraphics.stroke();
};

DialControl.prototype.PaintActivityButton = function(width, height) {
    if (this.GetActionButtonCount() === 0) return;
    var radius = this.GetActivityButtonRadius(width, height);
    if (this.activityEnabled) {
        this.PaintActionButton(
            this.GetActivityButtonCenter(width, height),
            radius,
            this.enabled,
            this.active,
            this.GetActionButtonColor()
        );
    }
    if (this.levelMatchEnabled) {
        this.PaintLevelMatchButton(
            this.GetLevelMatchButtonCenter(width, height),
            radius
        );
    }
    if (this.onsetMatchEnabled) {
        this.PaintOnsetMatchButton(
            this.GetOnsetMatchButtonCenter(width, height),
            radius
        );
    }
    if (this.autoMatchEnabled) {
        this.PaintAutoMatchButton(
            this.GetAutoMatchButtonCenter(width, height),
            radius
        );
    }
    if (this.listenEnabled) {
        this.PaintActionButton(
            this.GetListenButtonCenter(width, height),
            radius,
            this.enabled,
            this.listen,
            InterfaceTheme.colors.secondaryAccent
        );
    }
};

DialControl.prototype.HandleDrag = function(y) {
    var delta = this.lastY - y;
    this.lastY = y;
    this.SetValue(
        this.activeIndex,
        this.rawValues[this.activeIndex] + delta * DialOptions.dragSensitivity,
        true
    );
};

DialControl.prototype.SetActiveIndex = function(opt, mod2) {
    var requestedIndex = opt ? 2 : (mod2 ? 1 : 0);
    this.activeIndex = Math.min(requestedIndex, this.values.length - 1);
};

DialControl.prototype.SetActive = function(value) {
    this.active = Number(value) !== 0;
    if (!this.active) this.isDragging = false;
    mgraphics.redraw();
};

DialControl.prototype.SetActivityEnabled = function(value) {
    this.activityEnabled = Number(value) !== 0;
    if (!this.activityEnabled) {
        this.active = true;
        this.isDragging = false;
    }
    mgraphics.redraw();
};

DialControl.prototype.SetListenEnabled = function(value) {
    this.listenEnabled = Number(value) !== 0;
    if (!this.listenEnabled) this.listen = false;
    mgraphics.redraw();
};

DialControl.prototype.SetLevelMatchEnabled = function(value) {
    this.levelMatchEnabled = Number(value) !== 0;
    if (!this.levelMatchEnabled) this.levelMatchActive = false;
    mgraphics.redraw();
};

DialControl.prototype.SetOnsetMatchEnabled = function(value) {
    this.onsetMatchEnabled = Number(value) !== 0;
    if (!this.onsetMatchEnabled) this.onsetMatchActive = false;
    mgraphics.redraw();
};

DialControl.prototype.SetAutoMatchEnabled = function(value) {
    this.autoMatchEnabled = Number(value) !== 0;
    if (!this.autoMatchEnabled) this.autoMatchActive = false;
    mgraphics.redraw();
};

DialControl.prototype.SetListen = function(value) {
    this.listen = Number(value) !== 0;
    mgraphics.redraw();
};

DialControl.prototype.ToggleActive = function() {
    this.SetActive(this.active ? 0 : 1);
    outlet(0, ["active", this.active ? 1 : 0]);
};

DialControl.prototype.ToggleListen = function() {
    if (!this.listenEnabled || !this.enabled) return;
    this.listen = !this.listen;
    outlet(0, ["listen", this.listen ? 1 : 0]);
    mgraphics.redraw();
};

DialControl.prototype.MatchLevel = function() {
    if (!this.levelMatchEnabled || !this.enabled || !this.active) return;
    this.levelMatchActive = !this.levelMatchActive;
    outlet(0, "levelMatch", this.levelMatchActive ? 1 : 0);
    mgraphics.redraw();
};

DialControl.prototype.MatchOnset = function() {
    if (!this.onsetMatchEnabled || !this.enabled || !this.active) return;
    this.onsetMatchActive = !this.onsetMatchActive;
    outlet(0, "onsetMatch", this.onsetMatchActive ? 1 : 0);
    mgraphics.redraw();
};

DialControl.prototype.MatchAuto = function() {
    if (!this.autoMatchEnabled || !this.enabled || !this.active) return;
    this.autoMatchActive = !this.autoMatchActive;
    outlet(0, "autoMatch", this.autoMatchActive ? 1 : 0);
    mgraphics.redraw();
};

DialControl.prototype.HandleClick = function(x, y, opt, mod2) {
    if (this.IsListenButtonPoint(x, y, mgraphics.size[0], mgraphics.size[1])) {
        this.ToggleListen();
        return;
    }
    if (this.IsLevelMatchButtonPoint(x, y, mgraphics.size[0], mgraphics.size[1])) {
        this.MatchLevel();
        return;
    }
    if (this.IsOnsetMatchButtonPoint(x, y, mgraphics.size[0], mgraphics.size[1])) {
        this.MatchOnset();
        return;
    }
    if (this.IsAutoMatchButtonPoint(x, y, mgraphics.size[0], mgraphics.size[1])) {
        this.MatchAuto();
        return;
    }
    if (this.IsActivityButtonPoint(x, y, mgraphics.size[0], mgraphics.size[1])) {
        this.ToggleActive();
        return;
    }
    if (!this.enabled || !this.active) return;
    var currentTime = new Date().getTime();
    this.SetActiveIndex(opt, mod2);
    if (currentTime - this.lastClickTime <= this.doubleClickInterval) {
        this.lastClickTime = 0;
        this.isDragging = false;
        this.ResetActiveValue();
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
    if (!this.enabled || !this.active) return;
    this.SetActiveIndex(opt, mod2);
    if (this.isDragging) this.HandleDrag(y);
};

DialControl.prototype.HandleRelease = function(x, y, button, modifiers, inTime, outTime) {
    this.isDragging = false;
    this.ScheduleDisplayedReset();
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
    if (normalizedMode === "none") {
        visualization.relativeValue = 0.0;
        visualization.peakValue = 0.0;
        visualization.smoothedValue = 0.0;
        visualization.reductionValue = 0.0;
    }
    visualization.mode = normalizedMode;
    visualization.value = normalizedMode === "signed"
        ? Math.max(-1.0, Math.min(1.0, numericValue))
        : this.ClampValue(numericValue);
    mgraphics.redraw();
};

DialControl.prototype.SetLevelVisualization = function(index, peakValue, smoothedValue) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.visualizations.length) return;
    var peak = Number(peakValue);
    var smoothed = Number(smoothedValue);
    if (!isFinite(peak) || !isFinite(smoothed)) return;
    var visualization = this.visualizations[position];
    visualization.mode = "level";
    visualization.peakValue = Math.max(-1.0, Math.min(1.0, peak));
    visualization.smoothedValue = Math.max(-1.0, Math.min(1.0, smoothed));
    mgraphics.redraw();
};

DialControl.prototype.SetReductionVisualization = function(
    index,
    value
) {
    var position = Number(index) - 1;
    if (position < 0 || position >= this.visualizations.length) return;
    var reduction = Number(value);
    if (!isFinite(reduction)) return;
    var visualization = this.visualizations[position];
    visualization.reductionValue = Math.max(0.0, Math.min(1.0, reduction));
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
    this.visualizations[position].peakValue = 0.0;
    this.visualizations[position].smoothedValue = 0.0;
    this.visualizations[position].reductionValue = 0.0;
    mgraphics.redraw();
};

DialControl.prototype.OutputValue = function(index) {
    if (index !== undefined) {
        var position = Number(index) - 1;
        if (position < 0 || position >= this.values.length) return;
        outlet(0, [position + 1, this.values[position]]);
        return;
    }
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

function getActivityEnabled() {
    return dialControl.activityEnabled ? 1 : 0;
}

function setActivityEnabled(value) {
    dialControl.SetActivityEnabled(value);
}

function getListenEnabled() {
    return dialControl.listenEnabled ? 1 : 0;
}

function setListenEnabled(value) {
    dialControl.SetListenEnabled(value);
}

function getLevelMatchEnabled() {
    return dialControl.levelMatchEnabled ? 1 : 0;
}

function setLevelMatchEnabled(value) {
    dialControl.SetLevelMatchEnabled(value);
}

function getOnsetMatchEnabled() {
    return dialControl.onsetMatchEnabled ? 1 : 0;
}

function setOnsetMatchEnabled(value) {
    dialControl.SetOnsetMatchEnabled(value);
}

function getAutoMatchEnabled() {
    return dialControl.autoMatchEnabled ? 1 : 0;
}

function setAutoMatchEnabled(value) {
    dialControl.SetAutoMatchEnabled(value);
}

function active(value) {
    dialControl.SetActive(value);
}

function activityEnabled(value) {
    dialControl.SetActivityEnabled(value);
}

function listen(value) {
    dialControl.SetListen(value);
}

function levelMatchEnabled(value) {
    dialControl.SetLevelMatchEnabled(value);
}

function levelMatch(value) {
    dialControl.levelMatchActive = Number(value) !== 0;
    mgraphics.redraw();
}

function onsetMatchEnabled(value) {
    dialControl.SetOnsetMatchEnabled(value);
}

function autoMatchEnabled(value) {
    dialControl.SetAutoMatchEnabled(value);
}

function onsetMatch(value) {
    dialControl.onsetMatchActive = Number(value) !== 0;
    mgraphics.redraw();
}

function autoMatch(value) {
    dialControl.autoMatchActive = Number(value) !== 0;
    mgraphics.redraw();
}

function enableActive() {
    dialControl.SetActive(1);
}

function disableActive() {
    dialControl.SetActive(0);
}

function defaultValue(index, value) {
    dialControl.SetDefaultValue(Number(index) - 1, Number(value));
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

function outputValue(index) {
    dialControl.OutputValue(index);
}

function set(index, value) {
    dialControl.SetIndexedValue(index, value);
}

function limits(index, minimum, maximum) {
    dialControl.SetLimits(index, minimum, maximum);
}

function displayRange(index, minimum, maximum, logarithmic, decimals, suffix) {
    dialControl.SetDisplayRange(
        index, minimum, maximum, logarithmic, decimals, suffix);
}

function step(index, value) {
    dialControl.SetStep(index, value);
}

function indicator(index, value) {
    dialControl.SetIndicator(index, value);
}

function levelVisualization(index, peakValue, smoothedValue) {
    dialControl.SetLevelVisualization(index, peakValue, smoothedValue);
}

function reductionVisualization(index, value) {
    dialControl.SetReductionVisualization(index, value);
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
    dialControl.HandleClick(x, y, opt, mod2);
    if (dialControl.active && dialControl.enabled
        && !dialControl.IsActivityButtonPoint(
            x, y, mgraphics.size[0], mgraphics.size[1])
        && !dialControl.IsListenButtonPoint(
            x, y, mgraphics.size[0], mgraphics.size[1])
        && !dialControl.IsLevelMatchButtonPoint(
            x, y, mgraphics.size[0], mgraphics.size[1])
        && !dialControl.IsOnsetMatchButtonPoint(
            x, y, mgraphics.size[0], mgraphics.size[1])
        && !dialControl.IsAutoMatchButtonPoint(
            x, y, mgraphics.size[0], mgraphics.size[1])) {
        dialControl.BeginDrag(y);
    }
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

function notifydeleted() { dialControl.Dispose(); }
