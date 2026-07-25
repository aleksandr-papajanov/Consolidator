var MeterVisualOptions = {
    layout: {
        baseWidth: 90,
        baseHeight: 139,
        minimumScale: 0.5,
        maximumScale: 3
    },
    fontFamily: "Arial",
    text: {
        titleSize: 8,
        labelSize: 7,
        valueSize: 7,
        analogValueSize: 8,
        widthFactor: 0.56,
        dbDecimals: 1,
        dbUnit: " dB",
        positivePrefix: "+"
    },
    colors: {
        background: { r: 0.055, g: 0.055, b: 0.055, a: 1.0 },
        surface: { r: 0.10, g: 0.10, b: 0.10, a: 1.0 },
        track: { r: 0.16, g: 0.16, b: 0.16, a: 1.0 },
        border: { r: 0.28, g: 0.28, b: 0.28, a: 1.0 },
        text: { r: 0.72, g: 0.72, b: 0.72, a: 0.95 },
        mutedText: { r: 0.46, g: 0.46, b: 0.46, a: 0.95 },
        before: { r: 0.33, g: 0.50, b: 0.62, a: 0.92 },
        after: { r: 0.12, g: 0.78, b: 0.92, a: 0.96 },
        reference: { r: 0.96, g: 0.96, b: 0.96, a: 0.90 },
        active: { r: 0.10, g: 0.78, b: 0.92, a: 1.0 }
    },
    renderer: {
        panelBorderWidth: 1.5,
        panelOffset: 0.5,
        panelInset: 1,
        minimumPanelSize: 0,
        referenceLineWidth: 1.5
    },
    gain: {
        horizontalPaddingRatio: 0.06,
        titleHeightRatio: 0.12,
        footerHeightRatio: 0.17,
        gapRatio: 0.055,
        minimumMeterWidth: 2,
        minimumMeterHeight: 4,
        titleOffsetRatio: 0.05,
        titleSizeRatio: 0.058,
        labelOffsetRatio: -0.02,
        valueOffsetRatio: 0.072,
        labelSize: 6,
        valueSize: 7,
        columnShare: 0.5,
        minimumDb: -60,
        maximumDb: 6,
        smoothing: 0
    },
    analog: {
        paddingRatio: 0.02,
        pivotPositionY: 0.90,
        minimumRadius: 6,
        radiusWidthFactor: 0.46,
        radiusHeightFactor: 0.85,
        verticalScale: 0.78,
        horizontalScale: 1,
        startAngle: -2.62,
        endAngle: -0.52,
        minorTickLength: 4,
        majorTickLength: 8,
        minorTickWidth: 1.5,
        majorTickWidth: 2.5,
        labelRadiusInset: 18,
        labelSize: 8,
        labelOffsetRatio: 0.014,
        needleWidth: 2.5,
        needleRadiusOffset: 5,
        minimumNeedleRadius: 2,
        hubSize: 4,
        valuePositionY: 0.99,
        valueSizeRatio: 0.058,
        smoothing: 0.68
    }
};

function MeterPalette() {
    this.background = MeterVisualOptions.colors.background;
    this.surface = MeterVisualOptions.colors.surface;
    this.track = MeterVisualOptions.colors.track;
    this.border = MeterVisualOptions.colors.border;
    this.text = MeterVisualOptions.colors.text;
    this.mutedText = MeterVisualOptions.colors.mutedText;
    this.before = MeterVisualOptions.colors.before;
    this.after = MeterVisualOptions.colors.after;
    this.reference = MeterVisualOptions.colors.reference;
    this.active = MeterVisualOptions.colors.active;
}

function MeterRenderer() {
    this.palette = new MeterPalette();
}

MeterRenderer.prototype.Clamp = function(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value));
};

MeterRenderer.prototype.GetScale = function(width, height) {
    var scale = Math.min(
        width / MeterVisualOptions.layout.baseWidth,
        height / MeterVisualOptions.layout.baseHeight);
    return this.Clamp(
        scale,
        MeterVisualOptions.layout.minimumScale,
        MeterVisualOptions.layout.maximumScale);
};

MeterRenderer.prototype.SetColor = function(color) {
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
};

MeterRenderer.prototype.DrawBackground = function(width, height) {
    this.SetColor(this.palette.background);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
};

MeterRenderer.prototype.DrawPanel = function(x, y, width, height) {
    this.SetColor(this.palette.surface);
    mgraphics.rectangle(x, y, width, height);
    mgraphics.fill();
    this.SetColor(this.palette.border);
    mgraphics.set_line_width(MeterVisualOptions.renderer.panelBorderWidth);
    mgraphics.rectangle(
        x + MeterVisualOptions.renderer.panelOffset,
        y + MeterVisualOptions.renderer.panelOffset,
        Math.max(MeterVisualOptions.renderer.minimumPanelSize, width - MeterVisualOptions.renderer.panelInset),
        Math.max(MeterVisualOptions.renderer.minimumPanelSize, height - MeterVisualOptions.renderer.panelInset));
    mgraphics.stroke();
};

MeterRenderer.prototype.DrawText = function(x, y, text, color, size) {
    this.SetColor(color || this.palette.text);
    mgraphics.select_font_face(MeterVisualOptions.fontFamily);
    mgraphics.set_font_size(size);
    mgraphics.move_to(x, y);
    mgraphics.show_text(text);
};

MeterRenderer.prototype.EstimateTextWidth = function(text, size) {
    return String(text).length * size * MeterVisualOptions.text.widthFactor;
};

MeterRenderer.prototype.DrawCenteredText = function(x, y, width, text, color, size) {
    this.DrawText(x + Math.max(0, (width - this.EstimateTextWidth(text, size)) * 0.5), y, text, color, size);
};

MeterRenderer.prototype.FormatDb = function(value) {
    return (value >= 0 ? MeterVisualOptions.text.positivePrefix : "")
        + value.toFixed(MeterVisualOptions.text.dbDecimals)
        + MeterVisualOptions.text.dbUnit;
};

function GainMeterView(title, beforeIndex, afterIndex) {
    this.title = title;
    this.beforeIndex = beforeIndex;
    this.afterIndex = afterIndex;
    this.renderer = new MeterRenderer();
    this.levels = [-120, -120, -120, -120, -120];
    this.smoothing = MeterVisualOptions.gain.smoothing;
    this.minimumDb = MeterVisualOptions.gain.minimumDb;
    this.maximumDb = MeterVisualOptions.gain.maximumDb;
}

GainMeterView.prototype.SetLevels = function(values) {
    if (values.length !== 5) return;
    for (var index = 0; index < values.length; index += 1) {
        var value = Number(values[index]);
        if (!isFinite(value)) return;
        this.levels[index] = this.levels[index] * this.smoothing + value * (1 - this.smoothing);
    }
    mgraphics.redraw();
};

GainMeterView.prototype.Draw = function() {
    var size = mgraphics.size;
    var width = size[0];
    var height = size[1];
    var renderer = this.renderer;
    var scale = renderer.GetScale(width, height);
    var padding = width * MeterVisualOptions.gain.horizontalPaddingRatio;
    var titleHeight = height * MeterVisualOptions.gain.titleHeightRatio;
    var footerHeight = height * MeterVisualOptions.gain.footerHeightRatio;
    var gap = width * MeterVisualOptions.gain.gapRatio;
    var meterWidth = Math.max(
        MeterVisualOptions.gain.minimumMeterWidth * scale,
        (width - padding * 2 - gap) * MeterVisualOptions.gain.columnShare);
    var meterTop = titleHeight;
    var meterHeight = Math.max(
        MeterVisualOptions.gain.minimumMeterHeight * scale,
        height - meterTop - footerHeight);

    renderer.DrawBackground(width, height);
    renderer.DrawPanel(0, 0, width, height, scale);
    renderer.DrawCenteredText(
        0,
        height * MeterVisualOptions.gain.titleOffsetRatio,
        width,
        this.title,
        renderer.palette.text,
        Math.max(1, height * MeterVisualOptions.gain.titleSizeRatio));
    this.DrawColumn(padding, meterTop, meterWidth, meterHeight, "IN", this.levels[this.beforeIndex], renderer.palette.before, scale);
    this.DrawColumn(padding + meterWidth + gap, meterTop, meterWidth, meterHeight, "OUT", this.levels[this.afterIndex], renderer.palette.after, scale);
    this.DrawReference(padding, meterTop, meterWidth * 2 + gap, meterHeight, this.levels[4], scale);
};

GainMeterView.prototype.DrawColumn = function(x, y, width, height, label, levelDb, color, scale) {
    var renderer = this.renderer;
    var normalized = renderer.Clamp((levelDb - this.minimumDb) / (this.maximumDb - this.minimumDb), 0, 1);
    renderer.SetColor(renderer.palette.track);
    mgraphics.rectangle(x, y, width, height);
    mgraphics.fill();
    renderer.SetColor(color);
    mgraphics.rectangle(x, y + height * (1 - normalized), width, height * normalized);
    mgraphics.fill();
    renderer.DrawCenteredText(
        x,
        y + height * MeterVisualOptions.gain.labelOffsetRatio,
        width,
        label,
        renderer.palette.mutedText,
        Math.max(1, width * MeterVisualOptions.gain.labelSize / MeterVisualOptions.layout.baseWidth));
    renderer.DrawCenteredText(
        x,
        y + height + height * MeterVisualOptions.gain.valueOffsetRatio,
        width,
        renderer.FormatDb(levelDb),
        color,
        Math.max(1, width * MeterVisualOptions.gain.valueSize / MeterVisualOptions.layout.baseWidth));
};

GainMeterView.prototype.DrawReference = function(x, y, width, height, levelDb, scale) {
    var renderer = this.renderer;
    var normalized = renderer.Clamp((levelDb - this.minimumDb) / (this.maximumDb - this.minimumDb), 0, 1);
    var lineY = y + height * (1 - normalized);
    renderer.SetColor(renderer.palette.border);
    mgraphics.set_line_width(MeterVisualOptions.renderer.referenceLineWidth);
    mgraphics.move_to(x, lineY);
    mgraphics.line_to(x + width, lineY);
    mgraphics.stroke();
};

function AnalogMeterView(title, maximum, tickValues, valueFormatter, labelValues) {
    this.title = title;
    this.maximum = maximum;
    this.tickValues = tickValues;
    this.labelValues = labelValues || tickValues;
    this.valueFormatter = valueFormatter;
    this.renderer = new MeterRenderer();
    this.value = 0;
    this.smoothing = MeterVisualOptions.analog.smoothing;
}

AnalogMeterView.prototype.SetValue = function(value) {
    var finiteValue = Number(value);
    if (!isFinite(finiteValue)) return;
    this.value = this.value * this.smoothing + finiteValue * (1 - this.smoothing);
    mgraphics.redraw();
};

AnalogMeterView.prototype.Draw = function() {
    var size = mgraphics.size;
    var width = size[0];
    var height = size[1];
    var renderer = this.renderer;
    var scale = renderer.Clamp(
        width / MeterVisualOptions.layout.baseWidth,
        MeterVisualOptions.layout.minimumScale,
        MeterVisualOptions.layout.maximumScale);
    var padding = Math.min(width, height) * MeterVisualOptions.analog.paddingRatio;
    var contentWidth = Math.max(1, width - padding * 2);
    var contentHeight = Math.max(1, height - padding * 2);
    var startAngle = MeterVisualOptions.analog.startAngle;
    var endAngle = MeterVisualOptions.analog.endAngle;
    var pivotX = padding + contentWidth * 0.5;
    var pivotY = padding + contentHeight * MeterVisualOptions.analog.pivotPositionY;
    var radius = Math.max(
        MeterVisualOptions.analog.minimumRadius * scale,
        Math.min(
            contentWidth * MeterVisualOptions.analog.radiusWidthFactor,
            contentHeight * MeterVisualOptions.analog.radiusHeightFactor));
    var verticalScale = MeterVisualOptions.analog.verticalScale;

    renderer.DrawBackground(width, height);
    renderer.DrawPanel(0, 0, width, height, scale);
    renderer.SetColor(renderer.palette.border);
    mgraphics.set_line_width(MeterVisualOptions.analog.majorTickWidth);
    mgraphics.save();
    mgraphics.translate(pivotX, pivotY);
    mgraphics.scale(MeterVisualOptions.analog.horizontalScale, verticalScale);
    mgraphics.arc(0, 0, radius, startAngle, endAngle);
    mgraphics.stroke();
    mgraphics.restore();

    for (var index = 0; index < this.tickValues.length; index += 1) {
        var normalizedTick = this.tickValues.length === 1 ? 0 : index / (this.tickValues.length - 1);
        var angle = startAngle + normalizedTick * (endAngle - startAngle);
        var isMajorTick = this.labelValues.indexOf(this.tickValues[index]) >= 0;
        var tickLength = (isMajorTick
            ? MeterVisualOptions.analog.majorTickLength
            : MeterVisualOptions.analog.minorTickLength) * scale;
        var innerX = pivotX + Math.cos(angle) * (radius - tickLength);
        var innerY = pivotY + Math.sin(angle) * (radius - tickLength) * verticalScale;
        var outerX = pivotX + Math.cos(angle) * radius;
        var outerY = pivotY + Math.sin(angle) * radius * verticalScale;
        mgraphics.set_line_width(isMajorTick
            ? MeterVisualOptions.analog.majorTickWidth
            : MeterVisualOptions.analog.minorTickWidth);
        mgraphics.move_to(innerX, innerY);
        mgraphics.line_to(outerX, outerY);
        mgraphics.stroke();
        if (isMajorTick) {
            var label = String(this.tickValues[index]);
            renderer.DrawText(
                pivotX + Math.cos(angle) * (radius - MeterVisualOptions.analog.labelRadiusInset * scale)
                    - renderer.EstimateTextWidth(label, MeterVisualOptions.analog.labelSize * scale) * 0.5,
                pivotY + Math.sin(angle) * (radius - MeterVisualOptions.analog.labelRadiusInset * scale)
                    * verticalScale + height * MeterVisualOptions.analog.labelOffsetRatio,
                label,
                renderer.palette.mutedText,
                Math.max(1, width * MeterVisualOptions.analog.labelSize / MeterVisualOptions.layout.baseWidth));
        }
    }

    var normalizedValue = renderer.Clamp(this.value / this.maximum, 0, 1);
    var needleAngle = startAngle + normalizedValue * (endAngle - startAngle);
    renderer.SetColor(renderer.palette.active);
    mgraphics.set_line_width(MeterVisualOptions.analog.needleWidth);
    mgraphics.move_to(pivotX, pivotY);
    mgraphics.line_to(
        pivotX + Math.cos(needleAngle) * Math.max(
            MeterVisualOptions.analog.minimumNeedleRadius * scale,
            radius - MeterVisualOptions.analog.needleRadiusOffset * scale),
        pivotY + Math.sin(needleAngle) * Math.max(
            MeterVisualOptions.analog.minimumNeedleRadius * scale,
            radius - MeterVisualOptions.analog.needleRadiusOffset * scale) * verticalScale);
    mgraphics.stroke();
    mgraphics.ellipse(
        pivotX - MeterVisualOptions.analog.hubSize * scale * 0.5,
        pivotY - MeterVisualOptions.analog.hubSize * scale * 0.5,
        MeterVisualOptions.analog.hubSize * scale,
        MeterVisualOptions.analog.hubSize * scale);
    mgraphics.fill();
    renderer.DrawCenteredText(
        0,
        padding + contentHeight * MeterVisualOptions.analog.valuePositionY,
        width,
        this.valueFormatter(this.value),
        renderer.palette.text,
        Math.max(1, width * MeterVisualOptions.analog.valueSize / MeterVisualOptions.layout.baseWidth));
};
