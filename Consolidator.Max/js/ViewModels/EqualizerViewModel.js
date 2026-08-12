function EqualizerViewModel(state) {
    this.state = state;
    this.bypass = new StateValueViewModel(state, "equalizer.bypass");
    this.solo = new StateValueViewModel(state, "equalizer.solo");
    this.currentBank = null;
    this.showBank(1);
}

EqualizerViewModel.prototype.getGlobalStateValues = function () {
    return [this.bypass, this.solo];
};

EqualizerViewModel.prototype.getCurrentBankStateValues = function () {
    return this.currentBank ? this.currentBank.getStateValues() : [];
};

EqualizerViewModel.prototype.showBank = function (bankId) {
    if (this.currentBank && this.currentBank.bankId === bankId) {
        return;
    }

    if (this.currentBank) {
        this.currentBank.destroy();
    }
    this.currentBank = new BankViewModel(this.state, bankId);
};

EqualizerViewModel.prototype.destroy = function () {
    this.bypass.destroy();
    this.solo.destroy();
    if (this.currentBank) {
        this.currentBank.destroy();
        this.currentBank = null;
    }
};
