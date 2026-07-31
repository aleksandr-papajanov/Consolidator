function ControlState() {
    this.enabled = true;
    this.active = false;
    this.loading = false;
    this.label = "";
    this.value = 0.0;
    this.valueColor = null;
};

ControlState.prototype.SetEnabled = function(value) {
    this.enabled = Number(value) !== 0;
};

ControlState.prototype.SetActive = function(value) {
    this.active = Number(value) !== 0;
};

ControlState.prototype.SetValue = function(value) {
    this.value = Number(value);
};
