function BankViewModel(state, bankId) {
    this.state = state;
    this.bankId = bankId;
    var prefix = "equalizer.bank." + bankId;
    this.solo = new StateValueViewModel(
        state,
        prefix + ".solo"
    );
    this.bypass = new StateValueViewModel(
        state,
        prefix + ".bypass"
    );
    this.group = new StateValueViewModel(state, "bank." + bankId + ".group");
    this.filters = [];

    for (var filterId = 1; filterId <= 7; filterId += 1) {
        this.filters.push(new FilterViewModel(state, bankId, filterId));
    }
}

BankViewModel.prototype.getStateValues = function () {
    var values = [this.solo, this.bypass, this.group];
    for (var index = 0; index < this.filters.length; index += 1) {
        values = values.concat(this.filters[index].getStateValues());
    }
    return values;
};

BankViewModel.prototype.destroy = function () {
    this.solo.destroy();
    this.bypass.destroy();
    this.group.destroy();
    for (var index = 0; index < this.filters.length; index += 1) {
        this.filters[index].destroy();
    }
    this.filters = [];
};
