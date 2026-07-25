autowatch = 1;
inlets = 1;
outlets = 1;

include("JS/BankListModel.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var model = new BankListModel();
var itemIds = [];
var itemStates = [];
var scrollOffset = 0;
var backgroundColor = [0.08, 0.08, 0.08, 1.0];
var activeColor = [0.25, 0.25, 0.25, 1.0];
var joinColor = [0.1, 0.62, 0.78, 1.0];
var textColor = [0.82, 0.82, 0.82, 1.0];
var activeTextColor = [1.0, 1.0, 1.0, 1.0];

function inletassist(index) {
    assist(index === 0
        ? "clear, append <name> <bankId> <bypass> <solo>, setstate <activeId> <joinCount> <joinIds...>"
        : "");
}

function outletassist(index) {
    assist(index === 0 ? "select <bankId>; joinselection <count> <bankIds...>" : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function clear() {
    model.Clear();
    itemIds = [];
    itemStates = [];
    scrollOffset = 0;
    refresh();
}

function append() {
    var values = arrayfromargs(arguments);
    if (values.length !== 4) return;
    model.Append(String(values[0]));
    itemIds.push(Number(values[1]));
    itemStates.push({ bypass: Number(values[2]) !== 0, solo: Number(values[3]) !== 0 });
    clampScrollOffset();
    refresh();
}

function setstate() {
    var values = arrayfromargs(arguments);
    if (values.length < 2) return;
    var activeIndex = itemIds.indexOf(Number(values[0]));
    var count = Number(values[1]);
    if (activeIndex < 0 || !isFinite(count) || count < 0 || values.length !== count + 2) return;
    var joinIndices = [];
    for (var valueIndex = 0; valueIndex < count; valueIndex++) {
        var joinIndex = itemIds.indexOf(Number(values[valueIndex + 2]));
        if (joinIndex < 0) return;
        joinIndices.push(joinIndex);
    }
    model.SetActive(activeIndex);
    model.SetJoinSelection(joinIndices);
    ensureVisible(activeIndex);
    refresh();
}

function onclick(x, y, button, cmd, shift, capslock, option, ctrl) {
    var localIndex = model.IndexAt(y);
    if (localIndex < 0) return;
    var index = localIndex + scrollOffset;
    if (cmd || ctrl) {
        model.ToggleJoin(index);
        EmitJoinSelection();
    }
    else {
        model.SetActive(index);
        model.SetJoinSelection([]);
        outlet(0, "select", itemIds[index]);
    }
    refresh();
}

function EmitJoinSelection() {
    var indices = model.JoinIndices();
    var bankIds = [];
    for (var index = 0; index < indices.length; index++) bankIds.push(itemIds[indices[index]]);
    outlet(0, ["joinselection", bankIds.length].concat(bankIds));
}

function onwheel(delta) { scrollBy(delta > 0 ? 1 : -1); }
function onmousewheel(x, y, delta) { onwheel(delta); }

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

function DrawBadge(x, y, label, color) {
    mgraphics.set_source_rgba(color);
    mgraphics.rectangle(x, y, 11, 13);
    mgraphics.fill();
    mgraphics.set_source_rgba([0.05, 0.05, 0.05, 1.0]);
    mgraphics.move_to(x + 3, y + 10);
    mgraphics.show_text(label);
}

function paint() {
    var width = box.rect[2] - box.rect[0];
    var height = box.rect[3] - box.rect[1];

    mgraphics.set_source_rgba(backgroundColor);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();

    mgraphics.select_font_face("Ableton Sans", 0, 0);
    mgraphics.set_font_size(11);
    var end = Math.min(model.items.length, scrollOffset + visibleRowCount());
    for (var itemIndex = scrollOffset; itemIndex < end; itemIndex++) {
        var displayIndex = itemIndex - scrollOffset;
        var y = model.padding + displayIndex * model.rowHeight;
        if (itemIndex === model.activeIndex) {
            mgraphics.set_source_rgba(activeColor);
            mgraphics.rectangle(1, y, width - 2, model.rowHeight - 1);
            mgraphics.fill();
        }
        if (model.IsJoinSelected(itemIndex)) {
            mgraphics.set_source_rgba(joinColor);
            mgraphics.set_line_width(1);
            mgraphics.rectangle(1.5, y + 0.5, width - 3, model.rowHeight - 2);
            mgraphics.stroke();
        }
        mgraphics.set_source_rgba(itemIndex === model.activeIndex ? activeTextColor : textColor);
        mgraphics.move_to(8, y + 15);
        mgraphics.show_text(model.items[itemIndex]);
        var state = itemStates[itemIndex];
        if (state && state.solo) DrawBadge(width - 28, y + 4, "S", [0.1, 0.75, 0.9, 1.0]);
        if (state && state.bypass) DrawBadge(width - 14, y + 4, "B", [0.72, 0.45, 0.12, 1.0]);
    }
}

function refresh() { mgraphics.redraw(); }
