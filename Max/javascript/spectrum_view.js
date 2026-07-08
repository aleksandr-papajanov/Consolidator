autowatch = 1;
inlets = 5;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var curves = [[], [], [], [], []];

var minDb = -60;
var maxDb = 60;

// 0 = no temporal smoothing, 0.9 = very slow/smooth
var smoothing = 0.75;

var styles = [
    { r: 0.55, g: 0.55, b: 0.55, a: 1.0, width: 2.0 },
    { r: 0.35, g: 0.85, b: 1.00, a: 1.0, width: 2.0 },
    { r: 1.00, g: 0.35, b: 0.35, a: 1.0, width: 1.8 },
    { r: 0.75, g: 1.00, b: 0.35, a: 1.0, width: 1.6 },
    { r: 0.90, g: 0.45, b: 1.00, a: 1.0, width: 1.6 }
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

function range(minValue, maxValue) {
    minDb = Number(minValue);
    maxDb = Number(maxValue);
    mgraphics.redraw();
}

function paint() {
    var w = box.rect[2] - box.rect[0];
    var h = box.rect[3] - box.rect[1];

    drawBackground(w, h);
    drawGrid(w, h);
    drawZeroLine(w, h);

    for (var i = 0; i < curves.length; i++) {
        var s = styles[i];
        drawCurve(curves[i], w, h, s.r, s.g, s.b, s.a, s.width);
    }
}

function drawBackground(w, h) {
    mgraphics.set_source_rgba(0.07, 0.07, 0.07, 1);
    mgraphics.rectangle(0, 0, w, h);
    mgraphics.fill();
}

function drawGrid(w, h) {
    mgraphics.set_source_rgba(0.22, 0.22, 0.22, 1);
    mgraphics.set_line_width(1);

    for (var i = 0; i <= 6; i++) {
        var y = (h / 6) * i;
        mgraphics.move_to(0, y);
        mgraphics.line_to(w, y);
        mgraphics.stroke();
    }

    for (var j = 0; j <= 8; j++) {
        var x = (w / 8) * j;
        mgraphics.move_to(x, 0);
        mgraphics.line_to(x, h);
        mgraphics.stroke();
    }
}

function drawZeroLine(w, h) {
    var y = dbToY(0, h);

    mgraphics.set_source_rgba(0.45, 0.45, 0.45, 1);
    mgraphics.set_line_width(1.5);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

function drawCurve(values, w, h, r, g, b, a, width) {
    if (!values || values.length < 2) {
        return;
    }

    mgraphics.set_source_rgba(r, g, b, a);
    mgraphics.set_line_width(width);

    var points = values.map(function (v, i) {
        return {
            x: i / (values.length - 1) * w,
            y: dbToY(v, h)
        };
    });

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
    mgraphics.stroke();
}

function dbToY(db, h) {
    db = clamp(db, minDb, maxDb);
    var norm = (db - minDb) / (maxDb - minDb);
    return h - norm * h;
}

function clamp(v, min, max) {
    return Math.max(min, Math.min(max, v));
}
