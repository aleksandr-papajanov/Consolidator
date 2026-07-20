function BankListModel() {
    this.items = [];
    this.selectedIndex = -1;
    this.rowHeight = 22;
    this.padding = 5;
}

BankListModel.prototype.Clear = function() {
    this.items = [];
    this.selectedIndex = -1;
};

BankListModel.prototype.Append = function(name) {
    this.items.push(name);
};

BankListModel.prototype.Select = function(index) {
    this.selectedIndex = this.ClampIndex(index);
};

BankListModel.prototype.IndexAt = function(y) {
    var index = Math.floor((y - this.padding) / this.rowHeight);
    return index >= 0 && index < this.items.length ? index : -1;
};

BankListModel.prototype.ClampIndex = function(index) {
    index = Math.floor(Number(index));
    if (!isFinite(index) || this.items.length < 1) {
        return -1;
    }
    return Math.max(0, Math.min(index, this.items.length - 1));
};
