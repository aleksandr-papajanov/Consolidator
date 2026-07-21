autowatch = 1;
inlets = 1;
outlets = 0;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

function CompressorView() {
    this.bypass = false;
    this.thresholdDb = -18;
    this.minimumDb = -60;
    this.ratio = 4;
}

CompressorView.prototype.SetState = function(values) {
    if (values.length !== 4) return;
    this.bypass = Number(values[0]) !== 0;
    this.thresholdDb = Number(values[3]);
    mgraphics.redraw();
};

CompressorView.prototype.MapX = function(db, width) {
    return (db - this.minimumDb) / -this.minimumDb * width;
};

CompressorView.prototype.MapY = function(db, height) {
    return height - (db - this.minimumDb) / -this.minimumDb * height;
};

CompressorView.prototype.OutputDb = function(inputDb) {
    if (this.bypass || inputDb <= this.thresholdDb) return inputDb;
    return this.thresholdDb + (inputDb - this.thresholdDb) / this.ratio;
};

CompressorView.prototype.Paint = function() {
    var width = box.rect[2] - box.rect[0];
    var height = box.rect[3] - box.rect[1];
    var padding = 2;
    var graphWidth = Math.max(1, width - padding * 2);
    var graphHeight = Math.max(1, height - padding * 2);

    mgraphics.set_source_rgba([0.35, 0.35, 0.35, 0.45]);
    mgraphics.set_line_width(1);
    mgraphics.move_to(padding, height - padding);
    mgraphics.line_to(width - padding, padding);
    mgraphics.stroke();

    mgraphics.set_source_rgba(this.bypass
        ? [0.48, 0.48, 0.48, 0.8]
        : [0.25, 0.72, 0.95, 1.0]);
    mgraphics.set_line_width(1.7);
    for (var index = 0; index <= 32; index++) {
        var inputDb = this.minimumDb + (-this.minimumDb * index / 32);
        var x = padding + this.MapX(inputDb, graphWidth);
        var y = padding + this.MapY(this.OutputDb(inputDb), graphHeight);
        if (index === 0) mgraphics.move_to(x, y);
        else mgraphics.line_to(x, y);
    }
    mgraphics.stroke();

    if (!this.bypass) {
        var thresholdX = padding + this.MapX(this.thresholdDb, graphWidth);
        var thresholdY = padding + this.MapY(this.thresholdDb, graphHeight);
        mgraphics.set_source_rgba([0.25, 0.72, 0.95, 1.0]);
        mgraphics.ellipse(thresholdX - 1.8, thresholdY - 1.8, 3.6, 3.6);
        mgraphics.fill();
    }
};

var compressorView = new CompressorView();

function paint() { compressorView.Paint(); }
function compressor_state() { compressorView.SetState(arrayfromargs(arguments)); }
function inletassist() { assist("compressor_state <bypass> <attackMs> <releaseMs> <thresholdDb>"); }
setinletassist(-1, inletassist);
