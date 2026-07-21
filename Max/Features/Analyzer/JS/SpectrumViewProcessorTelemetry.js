SpectrumViewController.prototype.ProcessorTelemetry = function(
    compressorReductionDb,
    saturationNonlinearRatio,
    saturationLevelDeltaDb
) {
    var telemetry = spectrumState.processorTelemetry;
    var settings = spectrumState.visualSettings.processorTelemetry;
    var reduction = this.Clamp(-Number(compressorReductionDb) || 0, 0, settings.maximumReductionDb);
    var nonlinear = this.Clamp(Number(saturationNonlinearRatio) || 0, 0, 1);
    var levelDelta = Number(saturationLevelDeltaDb) || 0;
    var previousWeight = telemetry.initialized ? settings.smoothing : 0;
    var nextWeight = 1 - previousWeight;
    telemetry.compressorReductionDb = telemetry.compressorReductionDb * previousWeight + reduction * nextWeight;
    telemetry.saturationNonlinearRatio = telemetry.saturationNonlinearRatio * previousWeight + nonlinear * nextWeight;
    telemetry.saturationLevelDeltaDb = telemetry.saturationLevelDeltaDb * previousWeight + levelDelta * nextWeight;
    telemetry.initialized = true;
    mgraphics.redraw();
};

SpectrumViewController.prototype.DrawProcessorTelemetry = function(plotWidth, width, height) {
    var telemetry = spectrumState.processorTelemetry;
    var settings = spectrumState.visualSettings.processorTelemetry;
    this.DrawTelemetryPanel(plotWidth, width, height, settings);
    var panelWidth = Math.max(1, width - plotWidth);
    var padding = this.Clamp(panelWidth * 0.04, 4, 8);
    var contentTop = 18;
    var horizontal = panelWidth >= 150 || panelWidth >= height * 1.25;
    var meterWidth = Math.max(24, horizontal
        ? (panelWidth - padding * 3) * 0.5
        : panelWidth - padding * 2);
    var meterHeight = horizontal
        ? Math.max(36, Math.min(82, height - contentTop - 28))
        : Math.max(30, (height - contentTop - padding * 3) * 0.5 - 12);
    var compressorX = plotWidth + padding;
    var compressorY = contentTop;
    var saturatorX = horizontal ? compressorX + meterWidth + padding : compressorX;
    var saturatorY = horizontal ? contentTop : contentTop + meterHeight + padding + 12;
    this.DrawVintageMeter(
        compressorX,
        compressorY,
        meterWidth,
        meterHeight,
        "COMP",
        telemetry.compressorReductionDb,
        settings.maximumReductionDb,
        [0, 5, 10, 15, 20],
        "-" + telemetry.compressorReductionDb.toFixed(1) + " dB",
        settings);
    this.DrawVintageMeter(
        saturatorX,
        saturatorY,
        meterWidth,
        meterHeight,
        "SAT",
        telemetry.saturationNonlinearRatio * 100,
        settings.maximumSaturationPercent,
        [0, 10, 20, 30, 40],
        this.FormatLevelDelta(telemetry.saturationLevelDeltaDb),
        settings);
};

SpectrumViewController.prototype.DrawTelemetryPanel = function(plotWidth, width, height, settings) {
    mgraphics.set_source_rgba(
        settings.background.r,
        settings.background.g,
        settings.background.b,
        settings.background.a);
    mgraphics.rectangle(plotWidth, 0, width - plotWidth, height);
    mgraphics.fill();
    mgraphics.set_source_rgba(
        settings.separator.r,
        settings.separator.g,
        settings.separator.b,
        settings.separator.a);
    mgraphics.set_line_width(1);
    mgraphics.move_to(plotWidth + 0.5, 0);
    mgraphics.line_to(plotWidth + 0.5, height);
    mgraphics.stroke();
    this.DrawTelemetryText(plotWidth + 7, 12, "PROCESS", settings.text, 7);
};

SpectrumViewController.prototype.DrawVintageMeter = function(
    x,
    y,
    width,
    height,
    title,
    value,
    maximum,
    tickValues,
    footer,
    settings
) {
    var startAngle = -2.55;
    var endAngle = -0.59;
    var pivotX = x + width * 0.5;
    var pivotY = y + height - 8;
    var radius = Math.min(width * 0.38, height * 0.48);
    this.DrawTelemetryText(
        x + (width - this.EstimateLabelWidth(title)) * 0.5,
        y + 7,
        title,
        settings.text,
        7);

    mgraphics.set_source_rgba(settings.scale.r, settings.scale.g, settings.scale.b, settings.scale.a);
    mgraphics.set_line_width(1);
    mgraphics.arc(pivotX, pivotY, radius, startAngle, endAngle);
    mgraphics.stroke();

    for (var index = 0; index < tickValues.length; index++) {
        var normalizedTick = index / (tickValues.length - 1);
        var tickAngle = startAngle + normalizedTick * (endAngle - startAngle);
        var innerX = pivotX + Math.cos(tickAngle) * (radius - 4);
        var innerY = pivotY + Math.sin(tickAngle) * (radius - 4);
        var outerX = pivotX + Math.cos(tickAngle) * radius;
        var outerY = pivotY + Math.sin(tickAngle) * radius;
        mgraphics.move_to(innerX, innerY);
        mgraphics.line_to(outerX, outerY);
        mgraphics.stroke();
        var label = String(tickValues[index]);
        var labelX = pivotX + Math.cos(tickAngle) * (radius + 9) - this.EstimateLabelWidth(label) * 0.5;
        var labelY = pivotY + Math.sin(tickAngle) * (radius + 9) + 2;
        this.DrawTelemetryText(labelX, labelY, label, settings.text, 6);
    }

    var normalizedValue = this.Clamp(value / maximum, 0, 1);
    var needleAngle = startAngle + normalizedValue * (endAngle - startAngle);
    var needleX = pivotX + Math.cos(needleAngle) * (radius - 5);
    var needleY = pivotY + Math.sin(needleAngle) * (radius - 5);
    mgraphics.set_source_rgba(settings.needle.r, settings.needle.g, settings.needle.b, settings.needle.a);
    mgraphics.set_line_width(2);
    mgraphics.move_to(pivotX, pivotY);
    mgraphics.line_to(needleX, needleY);
    mgraphics.stroke();
    mgraphics.ellipse(pivotX - 2, pivotY - 2, 4, 4);
    mgraphics.fill();
    this.DrawTelemetryText(
        x + (width - this.EstimateLabelWidth(footer)) * 0.5,
        y + height + 10,
        footer,
        settings.text,
        7);
};

SpectrumViewController.prototype.FormatLevelDelta = function(value) {
    return (value >= 0 ? "+" : "") + value.toFixed(1) + " dB";
};

SpectrumViewController.prototype.DrawTelemetryText = function(x, y, text, color, size) {
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(x, y);
    mgraphics.show_text(text);
};
