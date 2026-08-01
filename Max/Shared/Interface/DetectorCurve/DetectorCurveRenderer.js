include("../../Configuration/InterfaceTheme.js");
include("DetectorCurveOptions.js");
include("../Curve/CurveRenderer.js");

function DetectorCurveRenderer() {
    this.curveRenderer = new CurveRenderer();
}

DetectorCurveRenderer.prototype.Clamp = function(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, Number(value)));
};

DetectorCurveRenderer.prototype.FrequencyToX = function(frequencyHz, width) {
    var options = detectorCurveOptions;
    var minimum = Math.log(options.minimumFrequencyHz);
    var maximum = Math.log(options.maximumFrequencyHz);
    var normalized = (Math.log(frequencyHz) - minimum) / (maximum - minimum);
    return options.padding + normalized * (width - options.padding * 2.0);
};

DetectorCurveRenderer.prototype.PointToFrequency = function(x, width) {
    var options = detectorCurveOptions;
    var usableWidth = Math.max(1.0, width - options.padding * 2.0);
    var ratio = this.Clamp((Number(x) - options.padding) / usableWidth, 0.0, 1.0);
    return options.minimumFrequencyHz * Math.pow(
        options.maximumFrequencyHz / options.minimumFrequencyHz,
        ratio
    );
};

DetectorCurveRenderer.prototype.PointToGain = function(y, plotHeight) {
    var options = detectorCurveOptions;
    var ratio = this.Clamp(
        (Number(y) - options.padding) / Math.max(1.0, plotHeight),
        0.0,
        1.0
    );
    return options.maximumDb - ratio * (options.maximumDb - options.minimumDb);
};

DetectorCurveRenderer.prototype.DbToY = function(db, plotHeight) {
    var options = detectorCurveOptions;
    var normalized = (this.Clamp(db, options.minimumDb, options.maximumDb) - options.minimumDb)
        / (options.maximumDb - options.minimumDb);
    return options.padding + (1.0 - normalized) * plotHeight;
};

DetectorCurveRenderer.prototype.BinToX = function(index, count, width) {
    var options = detectorCurveOptions;
    if (count < 2) return options.padding;
    var normalized = index / (count - 1);
    var frequencyHz = options.minimumFrequencyHz * Math.pow(
        options.maximumFrequencyHz / options.minimumFrequencyHz,
        normalized
    );
    return this.FrequencyToX(frequencyHz, width);
};

DetectorCurveRenderer.prototype.SetColor = function(color) {
    mgraphics.set_source_rgba(color[0], color[1], color[2], color[3]);
};

DetectorCurveRenderer.prototype.PaintGrid = function(width, plotHeight) {
    var options = detectorCurveOptions;
    var frequencies = [100.0, 1000.0, 10000.0];
    var labels = ["100", "1k", "10k"];
    mgraphics.set_line_width(options.gridLineWidth);
    this.SetColor(options.gridColor);

    var zeroY = this.DbToY(0.0, plotHeight);
    mgraphics.move_to(options.padding, zeroY);
    mgraphics.line_to(width - options.padding, zeroY);
    mgraphics.stroke();

    mgraphics.select_font_face(InterfaceTheme.typography.fontFamily);
    mgraphics.set_font_size(InterfaceTheme.typography.minimumSize);
    for (var index = 0; index < frequencies.length; index++) {
        var x = this.FrequencyToX(frequencies[index], width);
        mgraphics.move_to(x, options.padding);
        mgraphics.line_to(x, options.padding + plotHeight);
        mgraphics.stroke();
        var labelSize = mgraphics.text_measure(labels[index]);
        mgraphics.move_to(x - labelSize[0] * 0.5, plotHeight + 8.0);
        mgraphics.show_text(labels[index]);
    }
};

DetectorCurveRenderer.prototype.PaintCurve = function(model, filterIndex, width, plotHeight, total) {
    var options = detectorCurveOptions;
    var listening = !total && model.IsListening(filterIndex + 1);
    var sourceColor = total
        ? InterfaceTheme.colors.text
        : (listening ? InterfaceTheme.colors.secondaryAccent
            : (model.linkColor || options.filterColors[filterIndex]));
    var color = {
        r: sourceColor[0], g: sourceColor[1], b: sourceColor[2], a: sourceColor[3]
    };
    this.curveRenderer.Paint(
        total ? model.BuildTotalCurve() : model.BuildCurve(filterIndex),
        width,
        plotHeight,
        color,
        total ? options.totalLineWidth : (listening ? options.listenLineWidth : options.filterLineWidth),
        this,
        options.minimumDb,
        options.maximumDb
    );
};

DetectorCurveRenderer.prototype.PaintMarker = function(model, index, width, plotHeight) {
    var options = detectorCurveOptions;
    var filter = model.filters[index];
    if (!filter || !filter.definition || filter.frequencyHz === null ||
        filter.gainDb === null) return;
    var x = this.FrequencyToX(filter.frequencyHz, width);
    var y = this.DbToY(filter.gainDb, plotHeight);
    var size = mgraphics.size;
    var radius = options.markerHitRadius;
    var filterColor = model.linkColor || options.filterColors[index];
    var neutral = Math.abs(filter.gainDb) < 1.0e-12;
    var opacity = neutral ? options.neutralMarkerOpacity : 1.0;
    if (filter.bypass) opacity *= options.inactiveMarkerOpacity;
    var markerColor = [
        filterColor[0],
        filterColor[1],
        filterColor[2],
        filterColor[3] * opacity
    ];
    this.SetColor(markerColor);
    if (model.IsListening(index + 1)) {
        this.SetColor(InterfaceTheme.colors.secondaryAccent);
        mgraphics.set_line_width(options.listenLineWidth);
        mgraphics.ellipse(
            x - options.listenMarkerRadius,
            y - options.listenMarkerRadius,
            options.listenMarkerRadius * 2.0,
            options.listenMarkerRadius * 2.0
        );
        mgraphics.stroke();
        this.SetColor(markerColor);
    }
    mgraphics.ellipse(x - radius, y - radius, radius * 2.0, radius * 2.0);
    if (filter.bypass) mgraphics.stroke();
    else mgraphics.fill();
};

DetectorCurveRenderer.prototype.PaintLinkedMarkers = function(model, width, plotHeight) {
    var options = detectorCurveOptions;
    var color = InterfaceTheme.colors.textInactive;
    for (var key in model.linkedFilters) {
        if (!model.linkedFilters.hasOwnProperty(key)) continue;
        var filter = model.linkedFilters[key];
        if (!filter.enabled || !isFinite(filter.frequencyHz) || !isFinite(filter.gainDb)) continue;
        var x = this.FrequencyToX(filter.frequencyHz, width);
        var y = this.DbToY(filter.gainDb, plotHeight);
        this.SetColor([color[0], color[1], color[2], color[3] * options.linkedMarkerOpacity]);
        mgraphics.set_line_width(options.linkedMarkerLineWidth);
        var radius = options.linkedMarkerRadius;
        mgraphics.move_to(x - radius, y - radius);
        mgraphics.line_to(x + radius, y + radius);
        mgraphics.move_to(x + radius, y - radius);
        mgraphics.line_to(x - radius, y + radius);
        mgraphics.stroke();
    }
};

DetectorCurveRenderer.prototype.Paint = function(model) {
    var options = detectorCurveOptions;
    var size = mgraphics.size;
    var width = size[0];
    var plotHeight = Math.max(1.0, size[1] - options.labelHeight - options.padding * 2.0);
    this.PaintGrid(width, plotHeight);
    this.PaintCurve(model, 0, width, plotHeight, false);
    this.PaintCurve(model, 1, width, plotHeight, false);
    this.PaintCurve(model, 0, width, plotHeight, true);
    this.PaintLinkedMarkers(model, width, plotHeight);
    this.PaintMarker(model, 0, width, plotHeight);
    this.PaintMarker(model, 1, width, plotHeight);
};
