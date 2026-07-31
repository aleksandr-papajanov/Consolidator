function ControlLayout() {}

ControlLayout.prototype.Size = function() {
    var size = mgraphics.size;
    return { width: size[0], height: size[1] };
};

ControlLayout.prototype.Padding = function(size, ratio) {
    return Math.min(size.width, size.height) * ratio;
};

ControlLayout.prototype.FontSize = function(size, ratio, minimumSize) {
    return Math.max(minimumSize, Math.min(size.width, size.height) * ratio);
};
