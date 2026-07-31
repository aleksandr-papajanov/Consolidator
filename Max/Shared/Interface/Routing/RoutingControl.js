include("../../Configuration/InterfaceTheme.js");
include("RoutingOptions.js");
include("RoutingViewModel.js");
include("RoutingRenderer.js");

function RoutingControl() {
    this.viewModel = new RoutingViewModel();
    this.source = this.viewModel.source;
    this.channel = this.viewModel.channel;
    this.renderer = new RoutingRenderer();
}

RoutingControl.prototype.State = function(name) {
    if (name === "source") return this.source;
    if (name === "channel") return this.channel;
    return null;
};

RoutingControl.prototype.SetItems = function(name, values) {
    var state = this.State(name);
    if (!state) return;
    state.SetItems(values);
    state.ClampScroll(this.ColumnGeometry()[name === "source" ? 0 : 1].height - RoutingOptions.headerHeight,
        RoutingOptions.rowHeight);
    mgraphics.redraw();
};

RoutingControl.prototype.SetSelection = function(name, value) {
    var state = this.State(name);
    if (!state) return;
    state.SetSelection(value);
    mgraphics.redraw();
};

RoutingControl.prototype.SetEnabled = function(name, value) {
    var state = this.State(name);
    if (!state) return;
    state.enabled = Number(value) !== 0;
    mgraphics.redraw();
};

RoutingControl.prototype.ColumnGeometry = function() {
    var size = mgraphics.size;
    var gap = Math.min(RoutingOptions.columnGap, size[0] * 0.08);
    var width = Math.max(1, (size[0] - gap) * 0.5);
    return [
        { x: 0, y: 0, width: width, height: size[1] },
        { x: width + gap, y: 0, width: width, height: size[1] }
    ];
};

RoutingControl.prototype.Paint = function() {
    this.renderer.Paint(this);
};

RoutingControl.prototype.HandleClick = function(x, y) {
    var columns = this.ColumnGeometry();
    var columnIndex = x < columns[1].x ? 0 : 1;
    var state = columnIndex === 0 ? this.source : this.channel;
    var column = columns[columnIndex];
    if (!state.enabled || y < column.y + RoutingOptions.headerHeight) return;
    var row = Math.floor(
        (y - column.y - RoutingOptions.headerHeight + state.scrollOffset)
            / RoutingOptions.rowHeight
    );
    if (row < 0 || row >= state.items.length) return;
    state.SetSelection(row + 1);
    outlet(0, columnIndex === 0 ? "source" : "channel", row + 1);
    mgraphics.redraw();
};

RoutingControl.prototype.HandleWheel = function(x, y, deltaY) {
    var columns = this.ColumnGeometry();
    var columnIndex = x < columns[1].x ? 0 : 1;
    var state = columnIndex === 0 ? this.source : this.channel;
    var column = columns[columnIndex];
    if (!state.enabled || y < column.y + RoutingOptions.headerHeight) return;
    state.Scroll(
        Number(deltaY) * RoutingOptions.scrollStep,
        RoutingOptions.rowHeight,
        column.height - RoutingOptions.headerHeight
    );
    mgraphics.redraw();
};
