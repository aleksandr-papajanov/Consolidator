autowatch = 1;
inlets = 1;
outlets = 0;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

function SaturatorView() {
    this.bypass = false;
    this.saturation = 0;
}

SaturatorView.prototype.SetState = function(values) {
    if (values.length !== 2) return;
    this.bypass = Number(values[0]) !== 0;
    this.saturation = Math.max(0, Math.min(1, Number(values[1])));
    mgraphics.redraw();
};

SaturatorView.prototype.Shape = function(input) {
    if (this.bypass || this.saturation <= 0) return input;
    var drive = 1 + this.saturation * 9;
    var shaped = this.Tanh(input * drive) / this.Tanh(drive);
    return input + this.saturation * (shaped - input);
};

SaturatorView.prototype.Tanh = function(value) {
    var exponent = Math.exp(2 * value);
    return (exponent - 1) / (exponent + 1);
};

SaturatorView.prototype.Paint = function() {
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
        : [0.98, 0.55, 0.24, 1.0]);
    mgraphics.set_line_width(1.7);
    for (var index = 0; index <= 40; index++) {
        var input = -1 + 2 * index / 40;
        var output = this.Shape(input);
        var x = padding + (input + 1) * 0.5 * graphWidth;
        var y = padding + (1 - (output + 1) * 0.5) * graphHeight;
        if (index === 0) mgraphics.move_to(x, y);
        else mgraphics.line_to(x, y);
    }
    mgraphics.stroke();
};

var saturatorView = new SaturatorView();

function paint() { saturatorView.Paint(); }
function saturator_state() { saturatorView.SetState(arrayfromargs(arguments)); }
function inletassist() { assist("saturator_state <bypass> <saturation>"); }
setinletassist(-1, inletassist);
