autowatch = 1;
inlets = 1;
outlets = 1;

include("JS/BankListModel.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var model = new BankListModel();
var itemIds = [];
var scrollOffset = 0;
var backgroundColor = [0.08, 0.08, 0.08, 1.0];
var selectedColor = [0.25, 0.25, 0.25, 1.0];
var textColor = [0.82, 0.82, 0.82, 1.0];
var selectedTextColor = [1.0, 1.0, 1.0, 1.0];

function inletassist(index) {
    assist(index === 0
        ? "List commands: clear, append <name> <bankId>, setid <bankId>"
        : "");
}

function outletassist(index) {
    assist(index === 0 ? "Selected one-based bank ID" : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function clear() {
    model.Clear();
    itemIds = [];
    scrollOffset = 0;
    refresh();
}

function append() {
    var values = arrayfromargs(arguments);
    if (values.length !== 2) return;
    model.Append(String(values[0]));
    itemIds.push(Number(values[1]));
    clampScrollOffset();
    refresh();
}

function setid(bankId) {
    var id = Number(bankId);
    for (var index = 0; index < itemIds.length; index++) {
        if (itemIds[index] === id) {
            ensureVisible(index);
            model.Select(index - scrollOffset);
            refresh();
            return;
        }
    }
    model.Select(-1);
    refresh();
}

function onclick(x, y) {
    var localIndex = model.IndexAt(y);
    if (localIndex < 0) {
        return;
    }
    var index = localIndex + scrollOffset;
    model.Select(index - scrollOffset);
    outlet(0, itemIds[index]);
    refresh();
}

function onwheel(delta) {
    scrollBy(delta > 0 ? 1 : -1);
}

function onmousewheel(x, y, delta) {
    onwheel(delta);
}

function scrollBy(delta) {
    var maximum = Math.max(0, itemIds.length - visibleRowCount());
    scrollOffset = Math.max(0, Math.min(scrollOffset + delta, maximum));
    refresh();
}

function ensureVisible(index) {
    var rows = visibleRowCount();
    if (index < scrollOffset) scrollOffset = index;
    if (index >= scrollOffset + rows) scrollOffset = index - rows + 1;
    clampScrollOffset();
}

function clampScrollOffset() {
    var maximum = Math.max(0, itemIds.length - visibleRowCount());
    scrollOffset = Math.max(0, Math.min(scrollOffset, maximum));
}

function visibleRowCount() {
    var height = box.rect[3] - box.rect[1];
    return Math.max(1, Math.floor((height - model.padding * 2) / model.rowHeight));
}

function paint() {
    var width = box.rect[2] - box.rect[0];
    var height = box.rect[3] - box.rect[1];

    mgraphics.set_source_rgba(backgroundColor);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();

    mgraphics.select_font_face("Ableton Sans", 0, 0);
    mgraphics.set_font_size(11);
    var visibleRows = visibleRowCount();
    var end = Math.min(model.items.length, scrollOffset + visibleRows);
    for (var i = scrollOffset; i < end; i++) {
        var displayIndex = i - scrollOffset;
        var y = model.padding + displayIndex * model.rowHeight;
        if (displayIndex === model.selectedIndex) {
            mgraphics.set_source_rgba(selectedColor);
            mgraphics.rectangle(1, y, width - 2, model.rowHeight - 1);
            mgraphics.fill();
        }
        mgraphics.set_source_rgba(displayIndex === model.selectedIndex ? selectedTextColor : textColor);
        mgraphics.move_to(8, y + 15);
        mgraphics.show_text(model.items[i]);
    }

    if (model.items.length > visibleRows) {
        var trackHeight = height - model.padding * 2;
        var thumbHeight = Math.max(12, trackHeight * visibleRows / model.items.length);
        var travel = trackHeight - thumbHeight;
        var maximum = model.items.length - visibleRows;
        var thumbY = model.padding + (maximum > 0 ? travel * scrollOffset / maximum : 0);
        mgraphics.set_source_rgba([0.35, 0.35, 0.35, 1.0]);
        mgraphics.rectangle(width - 4, thumbY, 3, thumbHeight);
        mgraphics.fill();
    }
}

function refresh() {
    mgraphics.redraw();
}
