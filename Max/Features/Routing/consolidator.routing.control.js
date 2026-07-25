autowatch = 1;
inlets = 1;
outlets = 1;
mgraphics.init();
include("JS/InterfaceVisualConfig.js");

var RoutingControlOptions = {
    paddingRatio: 0.05,
    rowGapRatio: 0.04,
    popupVisibleRows: 4,
    arrowSizeRatio: 0.11
};

function RoutingMenuState(label) {
    this.label = label;
    this.items = ["None"];
    this.selection = 1;
    this.enabled = false;
}

RoutingMenuState.prototype.SetItems = function(values) {
    this.items = [];
    for (var index = 0; index < values.length; index++) {
        this.items.push(String(values[index]));
    }
    if (this.items.length === 0) this.items.push("None");
    this.selection = Math.max(1, Math.min(this.selection, this.items.length));
};

RoutingMenuState.prototype.SetSelection = function(value) {
    var index = Math.floor(Number(value));
    if (index >= 1 && index <= this.items.length) this.selection = index;
};

RoutingMenuState.prototype.SelectedText = function() {
    return this.items[this.selection - 1] || "None";
};

function RoutingControl() {
    this.source = new RoutingMenuState("Source");
    this.channel = new RoutingMenuState("Channel");
    this.openMenu = null;
    this.scrollOffset = 0;
}

RoutingControl.prototype.State = function(name) {
    if (name === "source") return this.source;
    if (name === "channel") return this.channel;
    return null;
};

RoutingControl.prototype.SetItems = function(name, values) {
    var state = this.State(name);
    if (!state) return;
    state.SetItems(values);
    this.ClampScroll();
    mgraphics.redraw();
};

RoutingControl.prototype.SetSelection = function(name, value) {
    var state = this.State(name);
    if (!state) return;
    state.SetSelection(value);
    mgraphics.redraw();
};

RoutingControl.prototype.SetEnabled = function(name, value) {
    var state = this.State(name);
    if (!state) return;
    state.enabled = Number(value) !== 0;
    if (!state.enabled && this.openMenu === name) this.ClosePopup();
    mgraphics.redraw();
};

RoutingControl.prototype.RowGeometry = function() {
    var size = mgraphics.size;
    var padding = Math.min(size[0], size[1]) * RoutingControlOptions.paddingRatio;
    var gap = Math.min(size[0], size[1]) * RoutingControlOptions.rowGapRatio;
    var rowHeight = (size[1] - padding * 2 - gap) * 0.5;
    return [
        { x: padding, y: padding, width: size[0] - padding * 2, height: rowHeight },
        { x: padding, y: padding + rowHeight + gap,
            width: size[0] - padding * 2, height: rowHeight }
    ];
};

RoutingControl.prototype.FitText = function(text, width, height) {
    var fontSize = Math.max(
        InterfaceVisualConfig.minimumTextFontSize,
        height * InterfaceVisualConfig.controlFontSizeRatio
    );
    mgraphics.set_font_size(fontSize);
    while (fontSize > InterfaceVisualConfig.minimumTextFontSize
        && mgraphics.text_measure(text)[0] > width) {
        fontSize -= 1;
        mgraphics.set_font_size(fontSize);
    }
    var fitted = String(text);
    while (fitted.length > 1 && mgraphics.text_measure(fitted)[0] > width) {
        fitted = fitted.substring(0, fitted.length - 1);
    }
    if (fitted !== text && fitted.length > 3) {
        fitted = fitted.substring(0, fitted.length - 3) + "...";
    }
    return fitted;
};

RoutingControl.prototype.DrawText = function(text, rect, color, reserveArrow) {
    var arrowWidth = reserveArrow ? rect.height : 0;
    var maximumWidth = Math.max(1, rect.width - rect.height * 0.3 - arrowWidth);
    var fittedText = this.FitText(text, maximumWidth, rect.height);
    var textSize = mgraphics.text_measure(fittedText);
    mgraphics.set_source_rgba(color);
    mgraphics.move_to(
        rect.x + rect.height * 0.15,
        rect.y + (rect.height + textSize[1]) * 0.5
    );
    mgraphics.show_text(fittedText);
};

RoutingControl.prototype.DrawArrow = function(rect, enabled) {
    var size = Math.min(rect.width, rect.height) * RoutingControlOptions.arrowSizeRatio;
    var centerX = rect.x + rect.width - rect.height * 0.35;
    var centerY = rect.y + rect.height * 0.5;
    mgraphics.set_source_rgba(enabled
        ? InterfaceVisualConfig.valueColor
        : InterfaceVisualConfig.borderColor);
    mgraphics.set_line_width(InterfaceVisualConfig.controlLineWidth);
    mgraphics.move_to(centerX - size, centerY - size * 0.5);
    mgraphics.line_to(centerX, centerY + size * 0.5);
    mgraphics.line_to(centerX + size, centerY - size * 0.5);
    mgraphics.stroke();
};

RoutingControl.prototype.DrawClosedRow = function(state, rect) {
    mgraphics.set_source_rgba(InterfaceVisualConfig.trackColor);
    mgraphics.rectangle(rect.x, rect.y, rect.width, rect.height);
    mgraphics.fill();
    this.DrawText(
        state.SelectedText(),
        rect,
        state.enabled ? InterfaceVisualConfig.textColor : InterfaceVisualConfig.borderColor,
        true
    );
    this.DrawArrow(rect, state.enabled);
};

RoutingControl.prototype.VisibleRowCount = function(state) {
    return Math.max(1, Math.min(
        RoutingControlOptions.popupVisibleRows,
        state.items.length
    ));
};

RoutingControl.prototype.PopupGeometry = function(state) {
    var size = mgraphics.size;
    var padding = Math.min(size[0], size[1]) * RoutingControlOptions.paddingRatio;
    var visibleRows = this.VisibleRowCount(state);
    return {
        x: padding,
        y: padding,
        width: size[0] - padding * 2,
        height: size[1] - padding * 2,
        rowHeight: (size[1] - padding * 2) / visibleRows,
        visibleRows: visibleRows
    };
};

RoutingControl.prototype.DrawPopup = function(state) {
    var popup = this.PopupGeometry(state);
    mgraphics.set_source_rgba(InterfaceVisualConfig.backgroundColor);
    mgraphics.rectangle(0, 0, mgraphics.size[0], mgraphics.size[1]);
    mgraphics.fill();

    for (var row = 0; row < popup.visibleRows; row++) {
        var itemIndex = this.scrollOffset + row;
        if (itemIndex >= state.items.length) break;
        var rect = {
            x: popup.x,
            y: popup.y + row * popup.rowHeight,
            width: popup.width,
            height: popup.rowHeight
        };
        var selected = itemIndex + 1 === state.selection;
        mgraphics.set_source_rgba(selected
            ? InterfaceVisualConfig.valueColor
            : InterfaceVisualConfig.trackColor);
        mgraphics.rectangle(rect.x, rect.y, rect.width, rect.height);
        mgraphics.fill();
        this.DrawText(state.items[itemIndex], rect, InterfaceVisualConfig.textColor, false);
    }
};

RoutingControl.prototype.Paint = function() {
    mgraphics.select_font_face("Arial");
    if (this.openMenu) {
        this.DrawPopup(this.State(this.openMenu));
        return;
    }
    var rows = this.RowGeometry();
    this.DrawClosedRow(this.source, rows[0]);
    this.DrawClosedRow(this.channel, rows[1]);
};

RoutingControl.prototype.OpenPopup = function(name) {
    var state = this.State(name);
    if (!state || !state.enabled) return;
    this.openMenu = name;
    this.scrollOffset = 0;
    mgraphics.redraw();
};

RoutingControl.prototype.ClosePopup = function() {
    this.openMenu = null;
    this.scrollOffset = 0;
    mgraphics.redraw();
};

RoutingControl.prototype.ClampScroll = function() {
    if (!this.openMenu) return;
    var state = this.State(this.openMenu);
    var maximumOffset = Math.max(0, state.items.length - this.VisibleRowCount(state));
    this.scrollOffset = Math.max(0, Math.min(this.scrollOffset, maximumOffset));
};

RoutingControl.prototype.HandleClick = function(x, y) {
    if (!this.openMenu) {
        var rows = this.RowGeometry();
        if (y >= rows[0].y && y <= rows[0].y + rows[0].height) {
            this.OpenPopup("source");
        } else if (y >= rows[1].y && y <= rows[1].y + rows[1].height) {
            this.OpenPopup("channel");
        }
        return;
    }

    var state = this.State(this.openMenu);
    var popup = this.PopupGeometry(state);
    var row = Math.floor((y - popup.y) / popup.rowHeight);
    if (row < 0 || row >= popup.visibleRows) return;
    var index = this.scrollOffset + row + 1;
    if (index > state.items.length) return;
    var name = this.openMenu;
    state.SetSelection(index);
    this.ClosePopup();
    outlet(0, name, index);
};

RoutingControl.prototype.HandleWheel = function(deltaY) {
    if (!this.openMenu || Number(deltaY) === 0) return;
    this.scrollOffset += Number(deltaY) < 0 ? 1 : -1;
    this.ClampScroll();
    mgraphics.redraw();
};

var routingControl = new RoutingControl();

function source_items() {
    routingControl.SetItems("source", arrayfromargs(arguments));
}

function channel_items() {
    routingControl.SetItems("channel", arrayfromargs(arguments));
}

function source_selection(value) {
    routingControl.SetSelection("source", value);
}

function channel_selection(value) {
    routingControl.SetSelection("channel", value);
}

function source_enabled(value) {
    routingControl.SetEnabled("source", value);
}

function channel_enabled(value) {
    routingControl.SetEnabled("channel", value);
}

function paint() {
    routingControl.Paint();
}

function onclick(x, y) {
    routingControl.HandleClick(x, y);
}

function onmousewheel(x, y, delta) {
    routingControl.HandleWheel(delta);
}
