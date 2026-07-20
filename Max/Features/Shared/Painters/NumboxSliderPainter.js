function SliderRenderer() {
    this.trackThickness = 2;
    this.markerSize = 8;
    this.markerInnerSize = 4;
}

SliderRenderer.prototype.StyleColor = function(styleName, fallback) {
    try {
        var attributeName = box.attrname_forstylemap(styleName);
        var color = box.getattr(attributeName);
        if (color && color.length >= 4) {
            return color;
        }
    } catch (error) {
    }
    return fallback;
};

SliderRenderer.prototype.ObjectColor = function(attributeName, fallback) {
    try {
        var color = box.getattr(attributeName);
        if (color && color.length >= 4) {
            return color;
        }
    } catch (error) {
    }
    return fallback;
};

SliderRenderer.prototype.IsEnabled = function() {
    try {
        return box.getattr("active") !== 0;
    } catch (error) {
        return true;
    }
};

SliderRenderer.prototype.FillRoundedRect = function(x, y, width, height, radius) {
    if (width <= 0 || height <= 0) {
        return;
    }

    radius = Math.min(radius, width * 0.5, height * 0.5);
    mgraphics.rectangle(x + radius, y, width - radius * 2, height);
    mgraphics.fill();
    mgraphics.rectangle(x, y + radius, width, height - radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(x, y, radius * 2, radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(x + width - radius * 2, y, radius * 2, radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(x, y + height - radius * 2, radius * 2, radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(x + width - radius * 2, y + height - radius * 2, radius * 2, radius * 2);
    mgraphics.fill();
};

SliderRenderer.prototype.Paint = function() {
    var size = mgraphics.size;
    var width = size[0];
    var height = size[1];
    var value = Math.max(0, Math.min(1, box.getvalueof()[0]));
    var enabled = this.IsEnabled();
    var track = this.StyleColor("bordercolor", [0.18, 0.18, 0.18, 1.0]);
    var active = enabled
        ? this.ObjectColor("activeslidercolor", this.ObjectColor("activedialcolor", [0.447, 0.035, 0.718, 1.0]))
        : this.ObjectColor("inactivelcdcolor", this.ObjectColor("inactivecolor", [0.18, 0.18, 0.18, 1.0]));
    var left = 4;
    var right = width - 4;
    var trackY = Math.floor(height * 0.5) - this.trackThickness * 0.5;
    var trackWidth = Math.max(1, right - left);
    var handleX = left + value * trackWidth;

    mgraphics.set_source_rgba(active);
    this.FillRoundedRect(left, trackY, Math.max(0, handleX - left), this.trackThickness, this.trackThickness * 0.5);
    mgraphics.set_source_rgba(track);
    this.FillRoundedRect(handleX, trackY, Math.max(0, right - handleX), this.trackThickness, this.trackThickness * 0.5);
    this.FillRoundedRect(handleX - this.markerSize * 0.5, trackY + this.trackThickness * 0.5 - this.markerSize * 0.5, this.markerSize, this.markerSize, 2);
    mgraphics.set_source_rgba(active);
    this.FillRoundedRect(handleX - this.markerInnerSize * 0.5, trackY + this.trackThickness * 0.5 - this.markerInnerSize * 0.5, this.markerInnerSize, this.markerInnerSize, 1);
};

var sliderRenderer = new SliderRenderer();

function paint() {
    sliderRenderer.Paint();
}
