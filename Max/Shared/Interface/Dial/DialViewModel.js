function DialViewModel() {
    ControlState.call(this);
    this.values = [];
    this.limits = [];
    this.displayRanges = [];
    this.ringColors = [];
    this.indicators = [];
}

DialViewModel.prototype = Object.create(ControlState.prototype);
DialViewModel.prototype.constructor = DialViewModel;
