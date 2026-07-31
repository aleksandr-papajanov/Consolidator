autowatch = 1;
inlets = 1;
outlets = 1;
mgraphics.init();
include("../../Configuration/InterfaceTheme.js");
include("ButtonGroupOptions.js");
include("../Button/ButtonViewModel.js");
include("../Core/ControlState.js");
include("../Core/ControlLayout.js");
include("../Core/ControlRenderer.js");
include("ButtonGroupViewModel.js");
include("ButtonGroupRenderer.js");
include("ButtonGroupLayout.js");

declareattribute("count", "getcount", "setcount", 1);
declareattribute("layout", "getlayout", "setlayout", 1);
declareattribute("selectionMode", "getSelectionMode", "setSelectionMode", 1);
declareattribute("allowEmptySelection", "getAllowEmptySelection", "setAllowEmptySelection", 1);
declareattribute("buttonModes", "getButtonModes", "setButtonModes", 1);
declareattribute("labels", "getlabels", "setlabels", 1);
declareattribute("selection", "getselection", "setselection", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);
declareattribute("loadingIndex", "getLoadingIndex", "setLoadingIndex", 1);

function ButtonGroupControl() {
    this.labels = ["One", "Two", "Three"];
    this.buttons = [];
    this.pressedIndex = -1;
    this.loadingIndex = 0;
    this.enabled = true;
    this.viewState = new ControlState();
    this.layout = new ControlLayout();
    this.options = CreateButtonGroupOptions();
    this.viewModel = new ButtonGroupViewModel();
    this.renderer = new ButtonGroupRenderer();
    this.SetCount(this.options.count);
};

ButtonGroupControl.prototype.SetCount = function(count) {
    var nextCount = Math.max(1, Math.floor(Number(count)));
    this.options.count = nextCount;
    while (this.labels.length < nextCount) this.labels.push(String(this.labels.length + 1));
    this.labels = this.labels.slice(0, nextCount);
    var previousButtons = this.buttons;
    this.buttons = [];
    for (var i = 0; i < nextCount; i++) {
        var button = new ButtonViewModel(
            "toggle"
        );
        if (previousButtons[i]) button.SetValue(previousButtons[i].value);
        this.buttons.push(button);
    }
    if (this.pressedIndex >= nextCount) this.pressedIndex = -1;
    if (this.loadingIndex > nextCount) this.loadingIndex = 0;
    this.ApplyButtonModes();
    mgraphics.redraw();
};

ButtonGroupControl.prototype.SetLayout = function(layout) {
    if (layout === "horizontal" || layout === "vertical") {
        this.options.layout = layout;
        mgraphics.redraw();
    }
};

ButtonGroupControl.prototype.SetSelectionMode = function(mode) {
    if (mode !== "single" && mode !== "multiple" && mode !== "custom") return;
    this.options.selectionMode = mode;
    this.pressedIndex = -1;
    this.ApplyButtonModes();
    if (mode === "single") {
        var firstSelected = -1;
        for (var i = 0; i < this.buttons.length; i++) {
            if (this.buttons[i].IsActive()) {
                firstSelected = i;
                break;
            }
        }
        for (var j = 0; j < this.buttons.length; j++) {
            this.buttons[j].SetValue(j === firstSelected ? 1 : 0);
        }
        if (this.GetSelection() === 0 && !this.options.allowEmptySelection) {
            this.buttons[0].SetValue(1);
        }
    }
    mgraphics.redraw();
};

ButtonGroupControl.prototype.ApplyButtonModes = function() {
    for (var i = 0; i < this.buttons.length; i++) {
        var mode = this.options.selectionMode === "custom"
            ? this.options.buttonModes[i]
            : "toggle";
        this.buttons[i].SetMode(mode === "momentary" ? "momentary" : "toggle");
    }
};

ButtonGroupControl.prototype.SetAllowEmptySelection = function(value) {
    this.options.allowEmptySelection = Number(value) !== 0;
    if (this.options.selectionMode !== "single"
        || this.options.allowEmptySelection
        || this.GetSelection() !== 0) {
        mgraphics.redraw();
        return;
    }
    this.buttons[0].SetValue(1);
    mgraphics.redraw();
};

ButtonGroupControl.prototype.SetButtonModes = function(modes) {
    var nextModes = [];
    for (var i = 0; i < this.buttons.length; i++) {
        nextModes.push(modes[i] === "momentary" ? "momentary" : "toggle");
    }
    this.options.buttonModes = nextModes;
    this.ApplyButtonModes();
    mgraphics.redraw();
};

ButtonGroupControl.prototype.SetLabels = function(labels) {
    this.labels = labels.slice(0, this.options.count);
    while (this.labels.length < this.options.count) {
        this.labels.push(String(this.labels.length + 1));
    }
    mgraphics.redraw();
};

ButtonGroupControl.prototype.GetGeometry = function() {
    var size = this.layout.Size();
    return this.renderer.Cells(this,
        { x: 0, y: 0, width: size.width, height: size.height }
    );
};

ButtonGroupControl.prototype.GetIndexAt = function(x, y) {
    return this.renderer.IndexAt(this, x, y);
};

ButtonGroupControl.prototype.EmitSelection = function() {
    var values = [];
    for (var i = 0; i < this.buttons.length; i++) {
        if (this.buttons[i].IsActive()) values.push(i + 1);
    }
    if (this.options.selectionMode === "single") {
        outlet(0, values.length > 0 ? values[0] : 0);
    } else {
        outlet(0, values);
    }
};

ButtonGroupControl.prototype.Select = function(index, shouldOutput) {
    if (!this.enabled && shouldOutput) return;
    if (index < 0 || index >= this.buttons.length) return;
    if (this.options.selectionMode === "custom") {
        this.pressedIndex = index;
        var customValue = this.buttons[index].HandleClick();
        mgraphics.redraw();
        if (shouldOutput) outlet(0, [index + 1, customValue]);
        return;
    }
    if (this.options.selectionMode === "single") {
        if (this.buttons[index].IsActive() && this.options.allowEmptySelection) {
            this.buttons[index].SetValue(0);
        } else {
            for (var i = 0; i < this.buttons.length; i++) {
                this.buttons[i].SetValue(i === index ? 1 : 0);
            }
        }
    } else {
        this.buttons[index].HandleClick();
    }
    mgraphics.redraw();
    if (shouldOutput) this.EmitSelection();
};

ButtonGroupControl.prototype.Paint = function() {
    this.renderer.Paint(this, this.GetGeometry());
};

ButtonGroupControl.prototype.HandleMessage = function(index) {
    this.Select(Number(index) - 1, false);
};

ButtonGroupControl.prototype.Release = function() {
    if (!this.enabled) return;
    if (this.pressedIndex < 0) return;
    var releasedIndex = this.pressedIndex;
    var releasedValue = this.buttons[releasedIndex].HandleRelease();
    this.pressedIndex = -1;
    mgraphics.redraw();
    if (releasedValue !== null) outlet(0, [releasedIndex + 1, releasedValue]);
};

var buttonGroupControl = new ButtonGroupControl();

function getcount() {
    return buttonGroupControl.options.count;
}

function setcount(value) {
    buttonGroupControl.SetCount(value);
}

function getlayout() {
    return buttonGroupControl.options.layout;
}

function setlayout(value) {
    buttonGroupControl.SetLayout(String(value));
}

function getSelectionMode() {
    return buttonGroupControl.options.selectionMode;
}

function setSelectionMode(value) {
    buttonGroupControl.SetSelectionMode(String(value));
}

function getAllowEmptySelection() {
    return buttonGroupControl.options.allowEmptySelection ? 1 : 0;
}

function setAllowEmptySelection(value) {
    buttonGroupControl.SetAllowEmptySelection(value);
}

function getButtonModes() {
    return buttonGroupControl.options.buttonModes;
}

function setButtonModes() {
    var values = arrayfromargs(arguments);
    if (values.length === 1 && typeof values[0] === "object") values = values[0];
    buttonGroupControl.SetButtonModes(values);
}

function getlabels() {
    return buttonGroupControl.labels;
}

function setlabels() {
    var values = arrayfromargs(arguments);
    if (values.length === 1 && typeof values[0] === "object") values = values[0];
    buttonGroupControl.SetLabels(values);
}

ButtonGroupControl.prototype.GetSelection = function() {
    var values = [];
    for (var i = 0; i < this.buttons.length; i++) {
        if (this.buttons[i].IsActive()) values.push(i + 1);
    }
    return this.options.selectionMode === "single"
        ? (values.length > 0 ? values[0] : 0)
        : values;
};

ButtonGroupControl.prototype.SetSelection = function(selection) {
    var values = typeof selection === "object" ? selection : [selection];
    for (var i = 0; i < this.buttons.length; i++) {
        this.buttons[i].SetValue(0);
    }
    for (var j = 0; j < values.length; j++) {
        var index = Number(values[j]) - 1;
        if (index >= 0 && index < this.buttons.length) this.buttons[index].SetValue(1);
    }
    if (this.options.selectionMode === "single"
        && !this.options.allowEmptySelection
        && this.GetSelection() === 0) {
        this.buttons[0].SetValue(1);
    }
    mgraphics.redraw();
};

function getselection() {
    return buttonGroupControl.GetSelection();
}

function setselection() {
    var values = arrayfromargs(arguments);
    if (values.length === 1 && typeof values[0] === "object") values = values[0];
    buttonGroupControl.SetSelection(
        buttonGroupControl.options.selectionMode === "single" ? values[0] : values
    );
}

function getEnabled() {
    return buttonGroupControl.enabled ? 1 : 0;
}

function setEnabled(value) {
    buttonGroupControl.SetEnabled(value);
}

ButtonGroupControl.prototype.SetEnabled = function(value) {
    var nextEnabled = Number(value) !== 0;
    if (!nextEnabled && this.pressedIndex >= 0) {
        var releasedIndex = this.pressedIndex;
        var releasedValue = this.buttons[releasedIndex].HandleRelease();
        this.pressedIndex = -1;
        if (releasedValue !== null) outlet(0, [releasedIndex + 1, releasedValue]);
    }
    this.enabled = nextEnabled;
    mgraphics.redraw();
};

ButtonGroupControl.prototype.SetLoadingIndex = function(value) {
    var nextIndex = Math.floor(Number(value));
    this.loadingIndex = nextIndex >= 1 && nextIndex <= this.buttons.length
        ? nextIndex
        : 0;
    mgraphics.redraw();
};

ButtonGroupControl.prototype.SetButtonValue = function(index, value) {
    var buttonIndex = Math.floor(Number(index)) - 1;
    if (buttonIndex < 0 || buttonIndex >= this.buttons.length) return;
    this.buttons[buttonIndex].SetValue(value);
    mgraphics.redraw();
};

function getLoadingIndex() {
    return buttonGroupControl.loadingIndex;
}

function setLoadingIndex(value) {
    buttonGroupControl.SetLoadingIndex(value);
}

function loadingIndex(value) {
    buttonGroupControl.SetLoadingIndex(value);
}

ButtonGroupControl.prototype.OutputValue = function() {
    if (this.options.selectionMode === "custom") {
        for (var i = 0; i < this.buttons.length; i++) {
            outlet(0, [i + 1, this.buttons[i].value]);
        }
        return;
    }
    this.EmitSelection();
};

function enabled(value) {
    buttonGroupControl.SetEnabled(value);
}

function enable() {
    buttonGroupControl.SetEnabled(1);
}

function disable() {
    buttonGroupControl.SetEnabled(0);
}

function outputValue() {
    buttonGroupControl.OutputValue();
}

function paint() {
    buttonGroupControl.Paint();
}

function count(value) {
    buttonGroupControl.SetCount(value);
}

function layout(value) {
    buttonGroupControl.SetLayout(String(value));
}

function selectionMode(value) {
    buttonGroupControl.SetSelectionMode(String(value));
}

function allowEmptySelection(value) {
    buttonGroupControl.SetAllowEmptySelection(value);
}

function buttonModes() {
    var values = arrayfromargs(arguments);
    buttonGroupControl.SetButtonModes(values);
}

function labels() {
    buttonGroupControl.SetLabels(arrayfromargs(arguments));
}

function set(value) {
    buttonGroupControl.HandleMessage(value);
}

function setvalue(index, value) {
    buttonGroupControl.SetButtonValue(index, value);
}

function onclick(x, y, button, modifiers, inTime, outTime) {
    buttonGroupControl.Select(buttonGroupControl.GetIndexAt(x, y), true);
}

function ondrag(x, y, button, modifiers, inTime, outTime) {
    if (button === 0) buttonGroupControl.Release();
}

function onidleout(x, y, button, modifiers, inTime, outTime) {
    buttonGroupControl.Release();
}

function msg_int(value) {
    buttonGroupControl.HandleMessage(value);
}

function msg_float(value) {
    buttonGroupControl.HandleMessage(value);
}

function onresize(width, height) {
    mgraphics.redraw();
}
