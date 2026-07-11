autowatch = 1;
inlets = 5;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var curves = [[], [], [], [], []];

var minDb = -30;
var maxDb = 30;
var dbRangePresets = [
    { min: -15, max: 15, label: "15 dB" },
    { min: -30, max: 30, label: "30 dB" }
];
var dbRangeIndex = 1;
var displayMinFrequency = 10;
var displayMaxFrequency = 20000;

var frequencyBands = [
    { min: 10, max: 100, width: 0.31 },
    { min: 100, max: 1000, width: 0.31 },
    { min: 1000, max: 10000, width: 0.31 },
    { min: 10000, max: 20000, width: 0.07 }
];

var frequencyLabels = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
var majorFrequencies = [100, 1000, 10000];
var minorFrequencies = [
    10, 20, 30, 40, 50, 60, 70, 80, 90,
    200, 300, 400, 500, 600, 700, 800, 900,
    2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000
];

// 0 = no temporal smoothing, 0.9 = very slow/smooth
var smoothing = 0.75;

var styles = [
    createCurveStyle(
        { r: 1.00, g: 1.00, b: 1.00, a: 0.60 },
        { r: 1.00, g: 1.00, b: 1.00, a: 0.1 },
        null
    ),
    createCurveStyle(
        { r: 1.00, g: 0.88, b: 0.25, a: 0.60 },
        { r: 1.00, g: 0.88, b: 0.25, a: 0.1 },
        null
    ),
    createCurveStyle(
        { r: 1.00, g: 1.00, b: 1.00, a: 0.00 },
        { r: 1.00, g: 1.00, b: 1.00, a: 0.00 },
        { r: 1.00, g: 0.35, b: 0.35, a: 0.95, width: 2.5 }
    ),
    createCurveStyle(
        { r: 0.75, g: 1.00, b: 0.35, a: 0.16 },
        { r: 0.75, g: 1.00, b: 0.35, a: 0.04 },
        { r: 0.75, g: 1.00, b: 0.35, a: 0.90, width: 1.5 }
    ),
    createCurveStyle(
        { r: 0.90, g: 0.45, b: 1.00, a: 0.16 },
        { r: 0.90, g: 0.45, b: 1.00, a: 0.04 },
        { r: 0.90, g: 0.45, b: 1.00, a: 0.90, width: 1.5 }
    )
];

function list() {
    var index = inlet;

    if (index < 0 || index >= curves.length) {
        return;
    }

    var incoming = arrayfromargs(arguments).map(Number);
    curves[index] = incoming;

    mgraphics.redraw();
}

function smooth(value) {
    smoothing = clamp(Number(value), 0, 0.98);
}

function clear() {
    for (var i = 0; i < curves.length; i++) {
        curves[i] = [];
    }

    mgraphics.redraw();
}

function target_size(size) {
    return;
}

function range(minValue, maxValue) {
    minDb = Number(minValue);
    maxDb = Number(maxValue);
    dbRangeIndex = -1;
    mgraphics.redraw();
}

function range_mode(value) {
    setDbRangeMode(Number(value));
}

function toggle_range() {
    setDbRangeMode(dbRangeIndex === 0 ? 1 : 0);
}

function setDbRangeMode(index) {
    if (index < 0 || index >= dbRangePresets.length) {
        return;
    }

    dbRangeIndex = index;
    minDb = dbRangePresets[index].min;
    maxDb = dbRangePresets[index].max;
    mgraphics.redraw();
}

function paint() {
    var w = box.rect[2] - box.rect[0];
    var h = box.rect[3] - box.rect[1];
    var plotBottom = getPlotBottom(h);

    drawBackground(w, h);
    drawFrequencyGrid(w, plotBottom);
    drawZeroLine(w, plotBottom);

    for (var i = 0; i < curves.length; i++) {
        var s = styles[i];
        drawCurve(curves[i], w, plotBottom, s);
    }

    drawFrequencyLabels(w, h);
    drawDbRangeLabel(w, h);
}

function drawBackground(w, h) {
    mgraphics.set_source_rgba(0.07, 0.07, 0.07, 1);
    mgraphics.rectangle(0, 0, w, h);
    mgraphics.fill();
}

function drawFrequencyGrid(w, plotBottom) {
    var gridStepDb = getDbGridStep();
    var maxGridDb = Math.floor(maxDb / gridStepDb) * gridStepDb;
    var minGridDb = -maxGridDb;

    for (var db = minGridDb; db <= maxGridDb; db += gridStepDb) {
        if (db === 0) {
            continue;
        }

        drawHorizontalGridLine(w, plotBottom, db, 0.22, 1);
        drawDbLabel(w, plotBottom, db);
    }

    drawDbLabel(w, plotBottom, 0);

    mgraphics.set_source_rgba(0.18, 0.18, 0.18, 1);
    for (var j = 0; j < minorFrequencies.length; j++) {
        var minorX = frequencyToX(minorFrequencies[j], w);
        mgraphics.set_line_width(0.8);
        mgraphics.move_to(minorX, 0);
        mgraphics.line_to(minorX, plotBottom);
        mgraphics.stroke();
    }

    mgraphics.set_source_rgba(0.28, 0.28, 0.28, 1);
    mgraphics.set_line_width(1.8);
    for (var k = 0; k < majorFrequencies.length; k++) {
        var boundaryX = frequencyToX(majorFrequencies[k], w);
        mgraphics.move_to(boundaryX, 0);
        mgraphics.line_to(boundaryX, plotBottom);
        mgraphics.stroke();
    }
}

function drawZeroLine(w, plotBottom) {
    var y = dbToY(0, plotBottom);

    mgraphics.set_source_rgba(0.45, 0.45, 0.45, 1);
    mgraphics.set_line_width(1.5);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

function drawCurve(values, w, plotBottom, style) {
    if (!values || values.length < 2) {
        return;
    }

    var points = values.map(function (v, i) {
        return {
            x: binToX(i, values.length, w),
            y: dbToY(v, plotBottom)
        };
    });

    if (style && style.fill) {
        drawFilledCurve(points, w, plotBottom, style.fill);
    }

    if (style && style.outline) {
        drawCurveOutline(points, style.outline);
    }
}

function drawFilledCurve(points, w, plotBottom, fillStyle) {
    var gradient = createVerticalGradient(0, 0, 0, plotBottom, fillStyle);

    mgraphics.new_path();
    mgraphics.move_to(0, plotBottom);
    mgraphics.line_to(points[0].x, points[0].y);

    for (var i = 1; i < points.length - 1; i++) {
        var midX = (points[i].x + points[i + 1].x) * 0.5;
        var midY = (points[i].y + points[i + 1].y) * 0.5;

        mgraphics.curve_to(
            points[i].x, points[i].y,
            points[i].x, points[i].y,
            midX, midY
        );
    }

    var last = points[points.length - 1];
    mgraphics.line_to(last.x, last.y);
    mgraphics.line_to(w, plotBottom);
    mgraphics.close_path();

    if (gradient && mgraphics.set_source) {
        mgraphics.set_source(gradient);
    } else {
        mgraphics.set_source_rgba(fillStyle.r, fillStyle.g, fillStyle.b, fillStyle.a);
    }

    mgraphics.fill();
}

function drawCurveOutline(points, outlineStyle) {
    mgraphics.new_path();
    mgraphics.move_to(points[0].x, points[0].y);

    for (var i = 1; i < points.length - 1; i++) {
        var midX = (points[i].x + points[i + 1].x) * 0.5;
        var midY = (points[i].y + points[i + 1].y) * 0.5;

        mgraphics.curve_to(
            points[i].x, points[i].y,
            points[i].x, points[i].y,
            midX, midY
        );
    }

    var last = points[points.length - 1];
    mgraphics.line_to(last.x, last.y);

    mgraphics.set_source_rgba(outlineStyle.r, outlineStyle.g, outlineStyle.b, outlineStyle.a);
    mgraphics.set_line_width(outlineStyle.width);
    mgraphics.stroke();
}

function drawFrequencyLabels(w, h) {
    var labelY = h - 4;

    mgraphics.set_source_rgba(0.8, 0.8, 0.8, 1);
    mgraphics.set_font_size(9);
    mgraphics.select_font_face("Arial");

    var previousRight = -9999;

    for (var i = 0; i < frequencyLabels.length; i++) {
        var freq = frequencyLabels[i];
        if (!isMajorFrequency(freq)) {
            continue;
        }

        var x = frequencyToX(freq, w);
        var label = formatFrequencyLabel(freq);
        var labelX = x - estimateLabelWidth(label) * 0.5;
        var labelRight = labelX + estimateLabelWidth(label);

        if (labelX < 0) {
            labelX = 0;
            labelRight = estimateLabelWidth(label);
        }

        if (labelRight > w) {
            labelX = w - estimateLabelWidth(label);
            labelRight = w;
        }

        if (labelX < previousRight + 6) {
            continue;
        }

        mgraphics.move_to(labelX, labelY);
        mgraphics.show_text(label);
        previousRight = labelRight;
    }
}

function drawDbRangeLabel(w, h) {
    var label = dbRangeIndex >= 0 ? dbRangePresets[dbRangeIndex].label : (Math.abs(maxDb) + " dB");
    var text = "-" + label + " / +" + label;

    mgraphics.set_source_rgba(0.65, 0.65, 0.65, 1);
    mgraphics.set_font_size(9);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(w - estimateLabelWidth(text) - 6, 12);
    mgraphics.show_text(text);
}

function createCurveStyle(fillTop, fillBottom, outline) {
    return {
        fill: {
            top: fillTop,
            bottom: fillBottom
        },
        outline: outline
    };
}

function createVerticalGradient(x0, y0, x1, y1, fillStyle) {
    if (!mgraphics.pattern_create_linear) {
        return null;
    }

    var pattern = mgraphics.pattern_create_linear(x0, y0, x1, y1);
    if (!pattern || !pattern.add_color_stop_rgba) {
        return null;
    }

    pattern.add_color_stop_rgba(0, fillStyle.top.r, fillStyle.top.g, fillStyle.top.b, fillStyle.top.a);
    pattern.add_color_stop_rgba(1, fillStyle.bottom.r, fillStyle.bottom.g, fillStyle.bottom.b, fillStyle.bottom.a);
    return pattern;
}

function drawHorizontalGridLine(w, plotBottom, db, gray, width) {
    var y = dbToY(db, plotBottom);
    mgraphics.set_source_rgba(gray, gray, gray, 1);
    mgraphics.set_line_width(width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

function drawDbLabel(w, plotBottom, db) {
    var text = formatDbLabel(db);
    var y = dbToY(db, plotBottom) + 3;

    mgraphics.set_source_rgba(0.72, 0.72, 0.72, 1);
    mgraphics.set_font_size(9);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(4, y);
    mgraphics.show_text(text);
}

function formatDbLabel(db) {
    if (db === 0) {
        return "0";
    }

    return (db > 0 ? "+" : "") + String(db);
}

function isMajorFrequency(freq) {
    for (var i = 0; i < majorFrequencies.length; i++) {
        if (majorFrequencies[i] === freq) {
            return true;
        }
    }

    return false;
}

function getDbGridStep() {
    return dbRangeIndex === 0 ? 6 : 12;
}

function frequencyToX(freq, w) {
    freq = clamp(freq, displayMinFrequency, displayMaxFrequency);

    var x = 0;

    for (var i = 0; i < frequencyBands.length; i++) {
        var band = frequencyBands[i];
        var bandWidth = w * band.width;

        if (freq <= band.max || i === frequencyBands.length - 1) {
            var t = normalizedLog(freq, band.min, band.max);
            return x + t * bandWidth;
        }

        x += bandWidth;
    }

    return w;
}

function binToX(index, total, w) {
    if (total <= 1) {
        return 0;
    }

    var t = index / (total - 1);
    var freq = displayMinFrequency * Math.pow(displayMaxFrequency / displayMinFrequency, t);
    return frequencyToX(freq, w);
}

function normalizedLog(value, minValue, maxValue) {
    value = clamp(value, minValue, maxValue);
    var logMin = Math.log(minValue);
    var logMax = Math.log(maxValue);
    return (Math.log(value) - logMin) / (logMax - logMin);
}

function formatFrequencyLabel(freq) {
    if (freq >= 1000) {
        return String(freq / 1000) + "k";
    }

    return String(freq);
}

function estimateLabelWidth(label) {
    return label.length * 4.6;
}

function getPlotBottom(h) {
    return h - 18;
}

function dbToY(db, plotBottom) {
    db = clamp(db, minDb, maxDb);
    var norm = (db - minDb) / (maxDb - minDb);
    return plotBottom - norm * plotBottom;
}

function clamp(v, min, max) {
    return Math.max(min, Math.min(max, v));
}
