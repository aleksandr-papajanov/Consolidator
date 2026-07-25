autowatch = 1;
inlets = 1;
outlets = 0;
mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;
include("../Interface/JS/InterfaceVisualConfig.js");

var DetectorCurveOptions = {
    minimumFrequencyHz: 20.0,
    maximumFrequencyHz: 20000.0,
    sampleRate: 48000.0,
    minimumDb: -24.0,
    maximumDb: 24.0,
    pointCount: 72,
    padding: 3.0,
    labelHeight: 8.0,
    gridLineWidth: 1.0,
    filterLineWidth: 1.0,
    totalLineWidth: 2.0,
    markerRadius: 2.5,
    labelFontSize: 7.0,
    gridColor: [0.28, 0.28, 0.28, 0.72],
    filterColors: [
        [0.10, 0.78, 0.92, 0.65],
        [0.98, 0.72, 0.18, 0.65]
    ]
};

function DetectorFilterState() {
    this.bypass = false;
    this.gainDb = 0.0;
    this.frequencyHz = 1000.0;
    this.q = 0.707;
}

function DetectorCurveView() {
    this.filters = [new DetectorFilterState(), new DetectorFilterState()];
}

DetectorCurveView.prototype.Clamp = function(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, Number(value)));
};

DetectorCurveView.prototype.FrequencyToX = function(frequencyHz, width) {
    var minimum = Math.log(DetectorCurveOptions.minimumFrequencyHz);
    var maximum = Math.log(DetectorCurveOptions.maximumFrequencyHz);
    var normalized = (Math.log(frequencyHz) - minimum) / (maximum - minimum);
    return DetectorCurveOptions.padding
        + normalized * (width - DetectorCurveOptions.padding * 2.0);
};

DetectorCurveView.prototype.DbToY = function(db, plotHeight) {
    var normalized = (this.Clamp(
        db,
        DetectorCurveOptions.minimumDb,
        DetectorCurveOptions.maximumDb
    ) - DetectorCurveOptions.minimumDb)
        / (DetectorCurveOptions.maximumDb - DetectorCurveOptions.minimumDb);
    return DetectorCurveOptions.padding + (1.0 - normalized) * plotHeight;
};

DetectorCurveView.prototype.MagnitudeDb = function(filter, frequencyHz) {
    if (filter.bypass || Math.abs(filter.gainDb) < 0.000001) return 0.0;

    var amplitude = Math.pow(10.0, filter.gainDb / 40.0);
    var omega = 2.0 * Math.PI * filter.frequencyHz
        / DetectorCurveOptions.sampleRate;
    var alpha = Math.sin(omega) / (2.0 * Math.max(0.01, filter.q));
    var cosine = Math.cos(omega);
    var b0 = 1.0 + alpha * amplitude;
    var b1 = -2.0 * cosine;
    var b2 = 1.0 - alpha * amplitude;
    var a0 = 1.0 + alpha / amplitude;
    var a1 = -2.0 * cosine;
    var a2 = 1.0 - alpha / amplitude;
    var evaluationOmega = 2.0 * Math.PI * frequencyHz
        / DetectorCurveOptions.sampleRate;
    var evaluationCosine = Math.cos(evaluationOmega);
    var evaluationSine = Math.sin(evaluationOmega);
    var doubleCosine = Math.cos(evaluationOmega * 2.0);
    var doubleSine = Math.sin(evaluationOmega * 2.0);
    var numeratorReal = b0 + b1 * evaluationCosine + b2 * doubleCosine;
    var numeratorImaginary = -b1 * evaluationSine - b2 * doubleSine;
    var denominatorReal = a0 + a1 * evaluationCosine + a2 * doubleCosine;
    var denominatorImaginary = -a1 * evaluationSine - a2 * doubleSine;
    var numeratorPower = numeratorReal * numeratorReal
        + numeratorImaginary * numeratorImaginary;
    var denominatorPower = denominatorReal * denominatorReal
        + denominatorImaginary * denominatorImaginary;
    return 10.0 * Math.log(Math.max(1.0e-12, numeratorPower / denominatorPower))
        / Math.LN10;
};

DetectorCurveView.prototype.PaintGrid = function(width, plotHeight) {
    var frequencies = [100.0, 1000.0, 10000.0];
    var labels = ["100", "1k", "10k"];
    mgraphics.set_line_width(DetectorCurveOptions.gridLineWidth);
    mgraphics.set_source_rgba(DetectorCurveOptions.gridColor);

    var zeroY = this.DbToY(0.0, plotHeight);
    mgraphics.move_to(DetectorCurveOptions.padding, zeroY);
    mgraphics.line_to(width - DetectorCurveOptions.padding, zeroY);
    mgraphics.stroke();

    mgraphics.select_font_face("Arial");
    mgraphics.set_font_size(DetectorCurveOptions.labelFontSize);
    for (var index = 0; index < frequencies.length; index++) {
        var x = this.FrequencyToX(frequencies[index], width);
        mgraphics.move_to(x, DetectorCurveOptions.padding);
        mgraphics.line_to(x, DetectorCurveOptions.padding + plotHeight);
        mgraphics.stroke();
        var labelSize = mgraphics.text_measure(labels[index]);
        mgraphics.move_to(x - labelSize[0] * 0.5, plotHeight + 8.0);
        mgraphics.show_text(labels[index]);
    }
};

DetectorCurveView.prototype.PaintCurve = function(
    filterIndex,
    width,
    plotHeight,
    total
) {
    var color = total
        ? InterfaceVisualConfig.textColor
        : DetectorCurveOptions.filterColors[filterIndex];
    mgraphics.set_source_rgba(color);
    mgraphics.set_line_width(total
        ? DetectorCurveOptions.totalLineWidth
        : DetectorCurveOptions.filterLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.new_path();

    for (var point = 0; point < DetectorCurveOptions.pointCount; point++) {
        var normalized = point / (DetectorCurveOptions.pointCount - 1);
        var frequency = DetectorCurveOptions.minimumFrequencyHz * Math.pow(
            DetectorCurveOptions.maximumFrequencyHz
                / DetectorCurveOptions.minimumFrequencyHz,
            normalized
        );
        var db = total
            ? this.MagnitudeDb(this.filters[0], frequency)
                + this.MagnitudeDb(this.filters[1], frequency)
            : this.MagnitudeDb(this.filters[filterIndex], frequency);
        var x = this.FrequencyToX(frequency, width);
        var y = this.DbToY(db, plotHeight);
        if (point === 0) mgraphics.move_to(x, y);
        else mgraphics.line_to(x, y);
    }
    mgraphics.stroke();
};

DetectorCurveView.prototype.PaintMarker = function(index, width, plotHeight) {
    var filter = this.filters[index];
    if (filter.bypass) return;
    var x = this.FrequencyToX(filter.frequencyHz, width);
    var y = this.DbToY(filter.gainDb, plotHeight);
    var radius = DetectorCurveOptions.markerRadius;
    mgraphics.set_source_rgba(DetectorCurveOptions.filterColors[index]);
    mgraphics.ellipse(x - radius, y - radius, radius * 2.0, radius * 2.0);
    mgraphics.fill();
};

DetectorCurveView.prototype.Paint = function() {
    var size = mgraphics.size;
    var width = size[0];
    var plotHeight = Math.max(
        1.0,
        size[1] - DetectorCurveOptions.labelHeight - DetectorCurveOptions.padding * 2.0
    );
    this.PaintGrid(width, plotHeight);
    this.PaintCurve(0, width, plotHeight, false);
    this.PaintCurve(1, width, plotHeight, false);
    this.PaintCurve(0, width, plotHeight, true);
    this.PaintMarker(0, width, plotHeight);
    this.PaintMarker(1, width, plotHeight);
};

DetectorCurveView.prototype.SetDetector = function(
    filterId,
    bypass,
    gainDb,
    frequencyHz,
    q
) {
    var index = Number(filterId) - 1;
    if (index < 0 || index >= this.filters.length) return;
    var filter = this.filters[index];
    filter.bypass = Number(bypass) !== 0;
    filter.gainDb = this.Clamp(gainDb, -24.0, 24.0);
    filter.frequencyHz = this.Clamp(frequencyHz, 20.0, 20000.0);
    filter.q = this.Clamp(q, 0.2, 8.0);
    mgraphics.redraw();
};

var detectorCurveView = new DetectorCurveView();

function paint() {
    detectorCurveView.Paint();
}

function detector(filterId, bypass, gainDb, frequencyHz, q) {
    detectorCurveView.SetDetector(filterId, bypass, gainDb, frequencyHz, q);
}

function onresize() {
    mgraphics.redraw();
}
