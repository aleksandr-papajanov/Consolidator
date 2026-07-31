include("CurveOptions.js");

function CurveRenderer() {
    this.options = new CurveOptions();
}

CurveRenderer.prototype.Paint = function(values, width, bottom, color, lineWidth, geometry, minimumDb, maximumDb) {
    if (!values || values.length < 2) return;
    minimumDb = minimumDb === undefined ? this.options.minimumDb : minimumDb;
    maximumDb = maximumDb === undefined ? this.options.maximumDb : maximumDb;
    lineWidth = lineWidth === undefined ? this.options.lineWidth : lineWidth;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(lineWidth);
    mgraphics.new_path();
    for (var index = 1; index < values.length; index++) {
        this.PaintSegment(values[index - 1], values[index],
            geometry.BinToX(index - 1, values.length, width),
            geometry.BinToX(index, values.length, width), bottom, geometry,
            minimumDb, maximumDb);
    }
    mgraphics.stroke();
};

CurveRenderer.prototype.PaintSegment = function(startValue, endValue, startX, endX,
    bottom, geometry, minimumDb, maximumDb) {
    var delta = endValue - startValue;
    if (delta === 0) {
        if (startValue < minimumDb || startValue > maximumDb) return;
        mgraphics.move_to(startX, geometry.DbToY(startValue, bottom));
        mgraphics.line_to(endX, geometry.DbToY(endValue, bottom));
        return;
    }

    var startT = 0;
    var endT = 1;
    if (startValue < minimumDb) {
        if (endValue <= minimumDb) return;
        startT = (minimumDb - startValue) / delta;
    } else if (startValue > maximumDb) {
        if (endValue >= maximumDb) return;
        startT = (maximumDb - startValue) / delta;
    }
    if (endValue < minimumDb) {
        if (startValue <= minimumDb) return;
        endT = (minimumDb - startValue) / delta;
    } else if (endValue > maximumDb) {
        if (startValue >= maximumDb) return;
        endT = (maximumDb - startValue) / delta;
    }
    if (startT > endT || endT < 0 || startT > 1) return;

    startT = Math.max(0, Math.min(1, startT));
    endT = Math.max(0, Math.min(1, endT));
    var clippedStart = startValue + delta * startT;
    var clippedEnd = startValue + delta * endT;
    mgraphics.move_to(startX + (endX - startX) * startT,
        geometry.DbToY(clippedStart, bottom));
    mgraphics.line_to(startX + (endX - startX) * endT,
        geometry.DbToY(clippedEnd, bottom));
};
