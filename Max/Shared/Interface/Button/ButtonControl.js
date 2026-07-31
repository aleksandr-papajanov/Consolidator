autowatch = 1;
inlets = 1;
outlets = 1;
mgraphics.init();
include("../../Configuration/InterfaceTheme.js");
include("ButtonOptions.js");
include("ButtonViewModel.js");
include("../Core/ControlState.js");
include("../Core/ControlLayout.js");
include("../Core/ControlRenderer.js");
include("ButtonRenderer.js");

declareattribute("mode", "getmode", "setmode", 1);
declareattribute("label", "getlabel", "setlabel", 1);
declareattribute("value", "getvalue", "setvalue", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);

function ButtonControl() {
    this.model = new ButtonViewModel(ButtonOptions.mode);
    this.isPressed = false;
    this.enabled = true;
    this.labelText = "Button";
    this.viewState = new ControlState();
    this.layout = new ControlLayout();
    this.renderer = new ButtonRenderer();
    this.viewState.label = this.labelText;
};

ButtonControl.prototype.SetValue = function(value, shouldOutput) {
    var nextValue = this.model.SetValue(value);
    this.viewState.SetValue(nextValue);
    this.viewState.SetActive(nextValue);
    mgraphics.redraw();
    if (shouldOutput) outlet(0, nextValue);
};

ButtonControl.prototype.SetMode = function(mode) {
    if (mode === "toggle" || mode === "momentary") {
        ButtonOptions.mode = mode;
        this.model.SetMode(mode);
        this.labelText = mode === "toggle" ? "Toggle" : "Momentary";
        this.viewState.label = this.labelText;
        mgraphics.redraw();
    }
};

ButtonControl.prototype.SetLabel = function(label) {
    this.labelText = String(label);
    this.viewState.label = this.labelText;
    mgraphics.redraw();
};

ButtonControl.prototype.Paint = function() {
    var size = this.layout.Size();
    this.viewState.enabled = this.enabled;
    this.viewState.active = this.model.IsActive();
    this.viewState.label = this.labelText;
    this.renderer.Paint(this.viewState, size, ButtonOptions, InterfaceTheme);
};

ButtonControl.prototype.HandleClick = function() {
    if (!this.enabled) return;
    this.isPressed = true;
    this.SetValue(this.model.HandleClick(), true);
};

ButtonControl.prototype.HandleRelease = function() {
    if (!this.enabled) return;
    this.isPressed = false;
    var value = this.model.HandleRelease();
    if (value !== null) this.SetValue(value, true);
};

ButtonControl.prototype.HandleMessage = function(value) {
    this.SetValue(value, false);
};

ButtonControl.prototype.SetEnabled = function(value) {
    this.enabled = Number(value) !== 0;
    this.viewState.SetEnabled(this.enabled);
    if (!this.enabled) this.isPressed = false;
    mgraphics.redraw();
};

ButtonControl.prototype.OutputValue = function() {
    outlet(0, this.model.value);
};

var buttonControl = new ButtonControl();

function getmode() {
    return buttonControl.model.mode;
}

function setmode(value) {
    buttonControl.SetMode(String(value));
}

function getlabel() {
    return buttonControl.labelText;
}

function setlabel(value) {
    buttonControl.SetLabel(value);
}

function getvalue() {
    return buttonControl.model.value;
}

function setvalue(value) {
    buttonControl.SetValue(value, false);
}

function getEnabled() {
    return buttonControl.enabled ? 1 : 0;
}

function setEnabled(value) {
    buttonControl.SetEnabled(value);
}

function enabled(value) {
    buttonControl.SetEnabled(value);
}

function enable() {
    buttonControl.SetEnabled(1);
}

function disable() {
    buttonControl.SetEnabled(0);
}

function outputValue() {
    buttonControl.OutputValue();
}

function set(value) {
    buttonControl.SetValue(value, false);
}

function paint() {
    buttonControl.Paint();
}

function msg_float(value) {
    buttonControl.HandleMessage(value);
}

function msg_int(value) {
    buttonControl.HandleMessage(value);
}

function bang() {
    buttonControl.HandleClick();
}

function mode(value) {
    buttonControl.SetMode(String(value));
}

function label() {
    var values = arrayfromargs(arguments);
    buttonControl.SetLabel(values.join(" "));
}

function list() {
    var values = arrayfromargs(arguments);
    if (values.length > 0) buttonControl.HandleMessage(values[0]);
}

function onclick(x, y, button, modifiers, inTime, outTime) {
    buttonControl.HandleClick();
}

function ondrag(x, y, button, modifiers, inTime, outTime) {
    if (button === 0) buttonControl.HandleRelease();
}

function onidleout(x, y, button, modifiers, inTime, outTime) {
    buttonControl.HandleRelease();
}

function onresize(width, height) {
    mgraphics.redraw();
}
