include("ButtonGroupOptions.js");
include("../../Configuration/InterfaceTheme.js");

function ButtonGroupLayout(options) {
    this.options = options || ButtonGroupOptions;
}

ButtonGroupLayout.prototype.Cells = function(rect, count, options) {
    if (!isFinite(count) || count <= 0) return [];
    var groupOptions = options || this.options;
    var horizontal = groupOptions.layout === "horizontal";
    var size = Math.min(rect.width, rect.height);
    var padding = size * groupOptions.paddingRatio;
    var gap = size * groupOptions.gapRatio;
    var available = (horizontal ? rect.width : rect.height)
        - padding * 2.0 - gap * (count - 1);
    var cellSize = Math.max(1.0, available / count);
    var cells = [];

    for (var index = 0; index < count; index++) {
        cells.push({
            x: horizontal ? rect.x + padding + index * (cellSize + gap) : rect.x + padding,
            y: horizontal ? rect.y + padding : rect.y + padding + index * (cellSize + gap),
            width: horizontal ? cellSize : rect.width - padding * 2.0,
            height: horizontal ? rect.height - padding * 2.0 : cellSize
        });
    }
    return cells;
};

ButtonGroupLayout.prototype.MeasureLabelWidth = function(label) {
    mgraphics.select_font_face(
        InterfaceTheme.typography.fontFamily,
        "normal",
        "normal"
    );
    mgraphics.set_font_size(InterfaceTheme.typography.minimumSize);
    return Math.max(
        InterfaceTheme.typography.minimumSize,
        mgraphics.text_measure(String(label))[0]
    );
};

ButtonGroupLayout.prototype.IndexAt = function(rect, count, x, y, options) {
    var cells = this.Cells(rect, count, options);
    for (var index = 0; index < cells.length; index++) {
        var cell = cells[index];
        if (x >= cell.x && x <= cell.x + cell.width &&
            y >= cell.y && y <= cell.y + cell.height) return index;
    }
    return -1;
};

ButtonGroupLayout.prototype.CellsByContent = function(rect, labels, options) {
    var groupOptions = options || this.options;
    var horizontal = groupOptions.layout === "horizontal";
    if (!horizontal || !labels || labels.length === 0) {
        return this.Cells(rect, labels ? labels.length : 0, groupOptions);
    }
    var padding = Math.min(rect.width, rect.height) * groupOptions.paddingRatio;
    var contentPadding = groupOptions.contentPadding || 0;
    var gap = Math.min(rect.width, rect.height) * groupOptions.gapRatio;
    var widths = [];
    var requestedWidth = 0;
    for (var index = 0; index < labels.length; index++) {
        var width = this.MeasureLabelWidth(labels[index]);
        widths.push(width + contentPadding * 2.0);
        requestedWidth += width + contentPadding * 2.0;
    }
    var available = Math.max(
        0,
        rect.width - padding * 2.0 - gap * (labels.length - 1)
    );
    if (requestedWidth > available && requestedWidth > 0) {
        var compression = available / requestedWidth;
        for (index = 0; index < widths.length; index++) {
            widths[index] *= compression;
        }
        requestedWidth = available;
    }
    var extra = Math.max(0, available - requestedWidth);
    var cells = [];
    var position = rect.x + padding;
    for (index = 0; index < widths.length; index++) {
        var weight = requestedWidth > 0 ? widths[index] / requestedWidth : 1 / widths.length;
        var cellWidth = widths[index] + extra * weight;
        cells.push({
            x: position,
            y: rect.y + padding,
            width: cellWidth,
            height: rect.height - padding * 2.0
        });
        position += cellWidth + gap;
    }
    if (cells.length > 0) {
        var finalCell = cells[cells.length - 1];
        finalCell.width = rect.x + rect.width - padding - finalCell.x;
    }
    return cells;
};

ButtonGroupLayout.prototype.ContentWidth = function(labels, height, options) {
    var groupOptions = options || this.options;
    if (groupOptions.layout !== "horizontal" || !labels || labels.length === 0) return 0;
    var size = Math.max(1, Number(height));
    var padding = size * groupOptions.paddingRatio;
    var gap = size * groupOptions.gapRatio;
    var contentPadding = groupOptions.contentPadding || 0;
    var width = padding * 2.0 + gap * Math.max(0, labels.length - 1);
    for (var index = 0; index < labels.length; ++index) {
        width += this.MeasureLabelWidth(labels[index]) + contentPadding * 2.0;
    }
    return width;
};

ButtonGroupLayout.prototype.IndexAtCells = function(rect, labels, x, y, options) {
    var cells = this.CellsByContent(rect, labels, options);
    for (var index = 0; index < cells.length; index++) {
        var cell = cells[index];
        if (x >= cell.x && x <= cell.x + cell.width &&
            y >= cell.y && y <= cell.y + cell.height) return index;
    }
    return -1;
};
