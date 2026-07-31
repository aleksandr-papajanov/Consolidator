function ButtonViewModel(mode) {
    this.mode = mode === "momentary" ? "momentary" : "toggle";
    this.value = 0;
};

ButtonViewModel.prototype.SetMode = function(mode) {
    if (mode === "toggle" || mode === "momentary") this.mode = mode;
};

ButtonViewModel.prototype.SetValue = function(value) {
    this.value = Number(value) === 1 ? 1 : 0;
    return this.value;
};

ButtonViewModel.prototype.HandleClick = function() {
    if (this.mode === "momentary") this.value = 1;
    else this.value = this.value ? 0 : 1;
    return this.value;
};

ButtonViewModel.prototype.HandleRelease = function() {
    if (this.mode !== "momentary") return null;
    this.value = 0;
    return this.value;
};

ButtonViewModel.prototype.IsActive = function() {
    return this.value === 1;
};
