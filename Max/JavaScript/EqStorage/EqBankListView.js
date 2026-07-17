autowatch = 1;
inlets = 1;
outlets = 1;

include("BankListModel.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var model = new BankListModel();
var backgroundColor = [0.08, 0.08, 0.08, 1.0];
var selectedColor = [0.25, 0.25, 0.25, 1.0];
var textColor = [0.82, 0.82, 0.82, 1.0];
var selectedTextColor = [1.0, 1.0, 1.0, 1.0];

function clear() {
    model.clear();
    refresh();
}

function append() {
    var values = arrayfromargs(arguments);
    if (values.length !== 1) return;
    model.append(String(values[0]));
    refresh();
}

function set(index) {
    model.select(Number(index));
    refresh();
}

function onclick(x, y) {
    var index = model.indexAt(y);
    if (index < 0) {
        return;
    }
    model.select(index);
    outlet(0, index);
    refresh();
}

function paint() {
    var width = box.rect[2] - box.rect[0];
    var height = box.rect[3] - box.rect[1];

    mgraphics.set_source_rgba(backgroundColor);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();

    mgraphics.select_font_face("Ableton Sans", 0, 0);
    mgraphics.set_font_size(11);
    for (var i = 0; i < model.items.length; i++) {
        var y = model.padding + i * model.rowHeight;
        if (i === model.selectedIndex) {
            mgraphics.set_source_rgba(selectedColor);
            mgraphics.rectangle(1, y, width - 2, model.rowHeight - 1);
            mgraphics.fill();
        }
        mgraphics.set_source_rgba(i === model.selectedIndex ? selectedTextColor : textColor);
        mgraphics.move_to(8, y + 15);
        mgraphics.show_text(model.items[i]);
    }
}

function refresh() {
    mgraphics.redraw();
}
