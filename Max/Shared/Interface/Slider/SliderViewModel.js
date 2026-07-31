function SliderViewModel() {
    ControlState.call(this);
    this.minimum = 0.0;
    this.maximum = 1.0;
    this.orientation = "horizontal";
    this.displayRange = null;
    this.valueColor = null;
}

SliderViewModel.prototype = Object.create(ControlState.prototype);
SliderViewModel.prototype.constructor = SliderViewModel;

SliderViewModel.prototype.SetOrientation = function(value) {
    this.orientation = value === "vertical" ? "vertical" : "horizontal";
};
