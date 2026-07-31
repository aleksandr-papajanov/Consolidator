function CurveViewModel() {
    this.values = [];
    this.color = { r: 1, g: 1, b: 1, a: 1 };
    this.lineWidth = 1;
}

CurveViewModel.prototype.Build = function(values, color, lineWidth) {
    this.values = values || [];
    this.color = color || this.color;
    this.lineWidth = lineWidth === undefined ? this.lineWidth : lineWidth;
    return this;
};
