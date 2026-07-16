autowatch = 1;
inlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var curve = [];
var minDb = -30;
var maxDb = 30;
var minFrequency = 20;
var maxFrequency = 20000;

function list() {
    curve = arrayfromargs(arguments).map(Number);
    mgraphics.redraw();
}

function clear() {
    curve = [];
    mgraphics.redraw();
}

function range(minValue, maxValue) {
    minDb = Number(minValue);
    maxDb = Number(maxValue);

    if (!isFinite(minDb) || !isFinite(maxDb) || minDb >= maxDb) {
        minDb = -30;
        maxDb = 30;
    }

    mgraphics.redraw();
}

function target_size() {
    mgraphics.redraw();
}

function onresize(width, height) {
    mgraphics.redraw();
    return;
}

function paint() {
    var w = box.rect[2] - box.rect[0];
    var h = box.rect[3] - box.rect[1];
    var plotBottom = h - 4;

    drawBackground(w, h);
    drawGrid(w, plotBottom);
    drawCurve(w, plotBottom);
}

function drawBackground(w, h) {
    mgraphics.set_source_rgba(0.07, 0.07, 0.07, 0);
    mgraphics.rectangle(0, 0, w, h);
    mgraphics.fill();
}

function drawGrid(w, plotBottom) {
    var frequencies = [100, 1000, 10000];

    mgraphics.set_source_rgba(0.24, 0.24, 0.24, 1);
    mgraphics.set_line_width(1);

    for (var i = 0; i < frequencies.length; i++) {
        var x = frequencyToX(frequencies[i], w);
        mgraphics.move_to(x, 0);
        mgraphics.line_to(x, plotBottom);
        mgraphics.stroke();
    }

    var dbLines = [minDb, 0, maxDb];
    for (var j = 0; j < dbLines.length; j++) {
        var y = dbToY(dbLines[j], plotBottom);
        mgraphics.set_source_rgba(dbLines[j] === 0 ? 0.42 : 0.18, 0.42, 0.42, 1);
        mgraphics.set_line_width(dbLines[j] === 0 ? 1.2 : 0.8);
        mgraphics.move_to(0, y);
        mgraphics.line_to(w, y);
        mgraphics.stroke();
    }

    drawLabels(w, plotBottom);
}

function drawLabels(w, plotBottom) {
    mgraphics.set_source_rgba(0.62, 0.62, 0.62, 1);
    mgraphics.set_font_size(8);
    mgraphics.select_font_face("Arial");

    var labels = [
        { frequency: 100, text: "100" },
        { frequency: 1000, text: "1k" },
        { frequency: 10000, text: "10k" }
    ];

    for (var i = 0; i < labels.length; i++) {
        var label = labels[i];
        var x = frequencyToX(label.frequency, w);
        mgraphics.move_to(x + 3, plotBottom - 3);
        mgraphics.show_text(label.text);
    }

    mgraphics.move_to(3, dbToY(maxDb, plotBottom) + 9);
    mgraphics.show_text("+" + maxDb);
    mgraphics.move_to(3, dbToY(0, plotBottom) - 3);
    mgraphics.show_text("0");
    mgraphics.move_to(3, dbToY(minDb, plotBottom) - 3);
    mgraphics.show_text(String(minDb));
}

function drawCurve(w, plotBottom) {
    if (curve.length < 2) {
        return;
    }

    mgraphics.new_path();
    mgraphics.move_to(binToX(0, curve.length, w), dbToY(curve[0], plotBottom));

    for (var i = 1; i < curve.length; i++) {
        mgraphics.line_to(
            binToX(i, curve.length, w),
            dbToY(curve[i], plotBottom)
        );
    }

    mgraphics.set_source_rgba(1.0, 0.78, 0.16, 1);
    mgraphics.set_line_width(2.5);
    mgraphics.stroke();
}

function frequencyToX(frequency, width) {
    var normalized = (Math.log(frequency) - Math.log(minFrequency)) /
        (Math.log(maxFrequency) - Math.log(minFrequency));
    return clamp(normalized, 0, 1) * width;
}

function binToX(index, total, width) {
    if (total <= 1) {
        return 0;
    }

    var normalized = index / (total - 1);
    var frequency = minFrequency * Math.pow(maxFrequency / minFrequency, normalized);
    return frequencyToX(frequency, width);
}

function dbToY(db, plotBottom) {
    var normalized = (clamp(db, minDb, maxDb) - minDb) / (maxDb - minDb);
    return plotBottom - normalized * plotBottom;
}

function clamp(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, value));
}
