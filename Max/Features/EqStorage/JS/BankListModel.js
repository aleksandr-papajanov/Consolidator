function BankListModel() {
    this.items = [];
    this.activeIndex = -1;
    this.joinIndices = {};
    this.rowHeight = 22;
    this.padding = 5;
}

BankListModel.prototype.Clear = function() {
    this.items = [];
    this.activeIndex = -1;
    this.joinIndices = {};
};

BankListModel.prototype.Append = function(name) {
    this.items.push(name);
};

BankListModel.prototype.SetActive = function(index) {
    this.activeIndex = this.ClampIndex(index);
};

BankListModel.prototype.SetJoinSelection = function(indices) {
    this.joinIndices = {};
    for (var index = 0; index < indices.length; index++) {
        var itemIndex = this.ClampIndex(indices[index]);
        if (itemIndex >= 0) this.joinIndices[itemIndex] = true;
    }
};

BankListModel.prototype.ToggleJoin = function(index) {
    index = this.ClampIndex(index);
    if (index < 0) return;
    if (this.joinIndices[index]) delete this.joinIndices[index];
    else this.joinIndices[index] = true;
};

BankListModel.prototype.IsJoinSelected = function(index) {
    return this.joinIndices[index] === true;
};

BankListModel.prototype.JoinIndices = function() {
    var result = [];
    for (var index = 0; index < this.items.length; index++) {
        if (this.IsJoinSelected(index)) result.push(index);
    }
    return result;
};

BankListModel.prototype.IndexAt = function(y) {
    var index = Math.floor((y - this.padding) / this.rowHeight);
    return index >= 0 && index < this.items.length ? index : -1;
};

BankListModel.prototype.ClampIndex = function(index) {
    index = Math.floor(Number(index));
    if (!isFinite(index) || this.items.length < 1) return -1;
    return Math.max(0, Math.min(index, this.items.length - 1));
};
