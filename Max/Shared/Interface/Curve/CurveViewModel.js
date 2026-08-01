include("../../Configuration/InterfaceTheme.js");

function CurveColor(value) {
    return { r: value[0], g: value[1], b: value[2], a: value[3] };
}

function CurveViewModel() {
    this.values = [];
    this.color = CurveColor(InterfaceTheme.colors.text);
    this.lineWidth = 1;
}

CurveViewModel.prototype.Build = function(values, color, lineWidth) {
    this.values = values || [];
    this.color = color || this.color;
    this.lineWidth = lineWidth === undefined ? this.lineWidth : lineWidth;
    return this;
};
