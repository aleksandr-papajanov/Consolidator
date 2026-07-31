include("../Button/ButtonRenderer.js");
include("ButtonGroupLayout.js");

function ButtonGroupRenderer() {
    this.buttonRenderer = new ButtonRenderer();
    this.layout = new ButtonGroupLayout();
}

ButtonGroupRenderer.prototype.Cells = function(group, rect) {
    var options = group.options || ButtonGroupOptions;
    return options.sizing === "equal"
        ? this.layout.Cells(rect, group.labels.length, options)
        : this.layout.CellsByContent(rect, group.labels, options);
};

ButtonGroupRenderer.prototype.IndexAt = function(group, x, y) {
    var cells = group.cells;
    if (!cells) return -1;
    for (var index = 0; index < cells.length; ++index) {
        var cell = cells[index];
        if (x >= cell.x && x <= cell.x + cell.width &&
            y >= cell.y && y <= cell.y + cell.height) return index;
    }
    return -1;
};

ButtonGroupRenderer.prototype.Paint = function(manager, cells) {
    manager.cells = cells;
    var options = manager.options || ButtonGroupOptions;
    var states = manager.viewModel.BuildStates(
        manager.buttons,
        manager.labels,
        manager.loadingIndex,
        manager.enabled,
        options.selectionMode,
        manager.pressedIndex,
        manager.visualStates
    );
    for (var index = 0; index < cells.length; index++) {
        this.buttonRenderer.PaintInRect(
            states[index],
            cells[index],
            options,
            InterfaceTheme
        );
    }
};
