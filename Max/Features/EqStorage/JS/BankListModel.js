function BankListModel() {
    this.items = [];
    this.selectedIndex = -1;
    this.rowHeight = 22;
    this.padding = 5;
}

BankListModel.prototype.clear = function() {
    this.items = [];
    this.selectedIndex = -1;
};

BankListModel.prototype.append = function(name) {
    this.items.push(name);
};

BankListModel.prototype.select = function(index) {
    this.selectedIndex = this.clampIndex(index);
};

BankListModel.prototype.indexAt = function(y) {
    var index = Math.floor((y - this.padding) / this.rowHeight);
    return index >= 0 && index < this.items.length ? index : -1;
};

BankListModel.prototype.clampIndex = function(index) {
    index = Math.floor(Number(index));
    if (!isFinite(index) || this.items.length < 1) {
        return -1;
    }
    return Math.max(0, Math.min(index, this.items.length - 1));
};
