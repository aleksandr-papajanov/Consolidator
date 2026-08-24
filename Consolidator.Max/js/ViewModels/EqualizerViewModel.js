function EqualizerViewModel(state) {
    this.state = state;
    this.bypass = new StateValueViewModel(state, "equalizer.bypass");
    this.bankBypass = new StateValueViewModel(state, "equalizer.bank.bypass");
    this.bankSolo = new StateValueViewModel(state, "equalizer.bank.solo");
    this.filters = [1, 2, 3, 4, 5, 6, 7].map(function (filterId) {
        return new FilterViewModel(state, filterId);
    });
};

EqualizerViewModel.prototype.destroy = function () {
    this.bypass.destroy();
    this.bankBypass.destroy();
    this.bankSolo.destroy();
    this.filters.forEach(function (filter) { filter.destroy(); });
    this.filters = [];
};
