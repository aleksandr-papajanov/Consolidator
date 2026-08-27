autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

class HistoryPanel
{
    constructor()
    {
        this.cursor = 0;
        this.entries = [];
        this.pending = null;
    }

    begin(cursor)
    {
        this.pending = { cursor: Number(cursor), entries: [] };
    }

    addEntry(index, kind, label, applied, current)
    {
        if (!this.pending) return;
        this.pending.entries[Number(index)] = {
            kind: String(kind), label: String(label),
            applied: Number(applied) !== 0, current: Number(current) !== 0
        };
    }

    end()
    {
        if (!this.pending) return;
        this.cursor = this.pending.cursor;
        this.entries = this.pending.entries;
        this.pending = null;
        mgraphics.redraw();
    }

    paint()
    {
        let width = mgraphics.size[0];
        let rowHeight = 22;
        mgraphics.set_source_rgba(0.06, 0.06, 0.06, 1);
        mgraphics.rectangle(0, 0, width, mgraphics.size[1]);
        mgraphics.fill();
        for (let i = 0; i < this.entries.length; i += 1) {
            let entry = this.entries[i];
            if (!entry) continue;
            let y = i * rowHeight;
            mgraphics.set_source_rgba(entry.current ? 0.2 : entry.applied ? 0.12 : 0.08,
                entry.current ? 0.45 : 0.08, entry.current ? 0.7 : 0.08, 1);
            mgraphics.rectangle(1, y + 1, width - 2, rowHeight - 2);
            mgraphics.fill();
            mgraphics.set_source_rgba(0.9, 0.9, 0.9, 1);
            mgraphics.select_font_face("Arial");
            mgraphics.set_font_size(11);
            mgraphics.move_to(6, y + 15);
            mgraphics.show_text(String(i + 1) + ". " + entry.label);
        }
    }

    select(y)
    {
        let index = Math.floor(y / 22);
        if (index < 0 || index >= this.entries.length || !this.entries[index]) return;
        outlet(0, ["historySelected", index + 1]);
    }
}

function presentation_begin(cursor) { historyPanel.begin(cursor); }
function entry(index, kind, label, applied, current) {
    historyPanel.addEntry(index, kind, label, applied, current);
}
function presentation_end() { historyPanel.end(); }
function paint() { historyPanel.paint(); }
function onresize() { mgraphics.redraw(); }
function onclick(x, y) { historyPanel.select(y); }
const historyPanel = new HistoryPanel();
