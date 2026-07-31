include("ListOptions.js");

function ListViewModel() {
    this.items = [];
    this.selection = 0;
    this.scrollOffset = 0;
}

ListViewModel.prototype.SetItems = function(values) {
    this.items = values ? values.slice() : [];
    if (this.items.length === 0) this.selection = 0;
    else if (this.selection < listOptions.minimumSelection || this.selection > this.items.length) {
        this.selection = listOptions.minimumSelection;
    }
    this.scrollOffset = Math.max(0, this.scrollOffset);
};

ListViewModel.prototype.SetSelection = function(value) {
    var index = Math.floor(Number(value));
    if (index >= listOptions.minimumSelection && index <= this.items.length) {
        this.selection = index;
    }
};

ListViewModel.prototype.SelectedItem = function() {
    return this.selection > 0 ? this.items[this.selection - 1] : null;
};

ListViewModel.prototype.MaximumScrollOffset = function(viewportHeight, rowHeight) {
    return Math.max(0, this.items.length * rowHeight - viewportHeight);
};

ListViewModel.prototype.ClampScroll = function(viewportHeight, rowHeight) {
    this.scrollOffset = Math.max(
        0,
        Math.min(this.MaximumScrollOffset(viewportHeight, rowHeight), this.scrollOffset)
    );
    return this.scrollOffset;
};

ListViewModel.prototype.Scroll = function(delta, rowHeight, viewportHeight) {
    var value = Number(delta);
    if (!isFinite(value) || value === 0) return this.scrollOffset;
    this.scrollOffset -= value * rowHeight;
    return this.ClampScroll(viewportHeight, rowHeight);
};
