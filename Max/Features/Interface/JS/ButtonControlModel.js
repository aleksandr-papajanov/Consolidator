function ButtonControlModel(mode) {
    this.mode = mode === "momentary" ? "momentary" : "toggle";
    this.value = 0;
};

ButtonControlModel.prototype.SetMode = function(mode) {
    if (mode === "toggle" || mode === "momentary") this.mode = mode;
};

ButtonControlModel.prototype.SetValue = function(value) {
    this.value = Number(value) === 1 ? 1 : 0;
    return this.value;
};

ButtonControlModel.prototype.HandleClick = function() {
    if (this.mode === "momentary") this.value = 1;
    else this.value = this.value ? 0 : 1;
    return this.value;
};

ButtonControlModel.prototype.HandleRelease = function() {
    if (this.mode !== "momentary") return null;
    this.value = 0;
    return this.value;
};

ButtonControlModel.prototype.IsActive = function() {
    return this.value === 1;
};
