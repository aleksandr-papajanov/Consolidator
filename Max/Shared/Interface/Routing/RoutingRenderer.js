include("../Core/ControlRenderer.js");
include("../List/ListRenderer.js");

function RoutingRenderer() {
    this.controlRenderer = new ControlRenderer();
    this.listRenderer = new ListRenderer();
}

RoutingRenderer.prototype.Paint = function(control) {
    var columns = control.ColumnGeometry();
    this.controlRenderer.SetColor(InterfaceTheme.colors.background);
    mgraphics.rectangle(0, 0, mgraphics.size[0], mgraphics.size[1]);
    mgraphics.fill();
    this.controlRenderer.SetFont(
        InterfaceTheme.typography.fontFamily,
        InterfaceTheme.typography.minimumSize
    );
    this.DrawColumn(control.source, columns[0]);
    this.DrawColumn(control.channel, columns[1]);
    this.DrawDivider(columns[0], columns[1]);
};

RoutingRenderer.prototype.DrawColumn = function(state, column) {
    this.DrawHeader(state.label, column);
    var viewport = {
        x: column.x,
        y: column.y + RoutingOptions.headerHeight,
        width: column.width,
        height: Math.max(0, column.height - RoutingOptions.headerHeight)
    };
    var rows = this.listRenderer.VisibleRows(
        state.items,
        state.scrollOffset,
        0,
        viewport.height,
        RoutingOptions.rowHeight
    );
    for (var index = 0; index < rows.length; ++index) {
        var row = rows[index];
        var selected = state.enabled && row.index + 1 === state.selection;
        var rect = {
            x: viewport.x,
            y: viewport.y + row.y,
            width: viewport.width,
            height: RoutingOptions.rowHeight
        };
        this.DrawRow(String(row.item), rect, selected, state.enabled);
    }
};

RoutingRenderer.prototype.DrawHeader = function(label, column) {
    this.controlRenderer.SetFont(
        InterfaceTheme.typography.fontFamily,
        InterfaceTheme.typography.minimumSize
    );
    this.controlRenderer.SetColor(InterfaceTheme.colors.textMuted);
    mgraphics.move_to(column.x + RoutingOptions.textPadding, column.y + RoutingOptions.headerHeight - 3);
    mgraphics.show_text(String(label).toUpperCase());
    this.controlRenderer.SetColor(InterfaceTheme.colors.borderInactive);
    mgraphics.set_line_width(InterfaceTheme.geometry.borderLineWidth);
    mgraphics.move_to(column.x, column.y + RoutingOptions.headerHeight - 1);
    mgraphics.line_to(column.x + column.width, column.y + RoutingOptions.headerHeight - 1);
    mgraphics.stroke();
};

RoutingRenderer.prototype.DrawRow = function(text, rect, selected, enabled) {
    var textColor = enabled
        ? (selected ? InterfaceTheme.states.active.text : InterfaceTheme.states.inactive.text)
        : InterfaceTheme.colors.textDisabled;
    if (selected) {
        this.controlRenderer.SetColor(InterfaceTheme.states.active.fill);
        mgraphics.rectangle(rect.x, rect.y, rect.width, rect.height);
        mgraphics.fill();
        this.controlRenderer.SetColor(InterfaceTheme.colors.secondaryAccent);
        mgraphics.rectangle(rect.x, rect.y, RoutingOptions.selectionBarWidth, rect.height);
        mgraphics.fill();
    }
    this.controlRenderer.DrawCenteredText(
        this.FitText(text, Math.max(1, rect.width - RoutingOptions.textPadding * 2)),
        {
            x: rect.x + RoutingOptions.textPadding,
            y: rect.y,
            width: Math.max(1, rect.width - RoutingOptions.textPadding * 2),
            height: rect.height
        },
        textColor,
        InterfaceTheme.typography.fontFamily,
        InterfaceTheme.typography.minimumSize
    );
};

RoutingRenderer.prototype.DrawDivider = function(left, right) {
    this.controlRenderer.SetColor(InterfaceTheme.colors.borderInactive);
    mgraphics.set_line_width(RoutingOptions.separatorWidth);
    mgraphics.move_to(right.x - RoutingOptions.columnGap * 0.5, left.y);
    mgraphics.line_to(right.x - RoutingOptions.columnGap * 0.5, left.y + left.height);
    mgraphics.stroke();
};

RoutingRenderer.prototype.FitText = function(text, width) {
    var fitted = String(text);
    while (fitted.length > 1 && mgraphics.text_measure(fitted)[0] > width) {
        fitted = fitted.substring(0, fitted.length - 1);
    }
    if (fitted !== String(text) && fitted.length > 3) {
        fitted = fitted.substring(0, fitted.length - 3) + "...";
    }
    return fitted;
};
