inlets = 1;
outlets = 0;

const PANEL_IDS = {
    input: "input-panel",
    saturator: "saturator-panel",
    compressor: "compressor-panel",
    equalizer: "eq-panel",
    output: "output-panel"
};

class PanelSelector
{
    constructor(patcher)
    {
        this.patcher = patcher;
        this.selected = "input";
    }

    setVisible(id, visible)
    {
        let box = this.patcher.getnamed(id);
        if (box && typeof box.setattr === "function") {
            box.setattr("hidden", visible ? 0 : 1);
        }
    }

    select(panel)
    {
        let name = String(panel || "").toLowerCase();
        if (!PANEL_IDS[name]) {
            return;
        }
        Object.keys(PANEL_IDS).forEach((key) => {
            this.setVisible(PANEL_IDS[key], key === name);
        });
        this.selected = name;
    }

    initialize()
    {
        this.select(this.selected);
    }
}

let panelSelector = null;

function ensurePanelSelector(patcher)
{
    if (!panelSelector && patcher) {
        panelSelector = new PanelSelector(patcher);
    }
    return panelSelector;
}

function loadbang()
{
    let selector = ensurePanelSelector(this.patcher);
    if (selector) {
        selector.initialize();
    }
}

function list(...args)
{
    let selector = ensurePanelSelector(this.patcher);
    if (selector && args.length > 0) {
        selector.select(args[0]);
    }
}

function anything(...args)
{
    let selector = ensurePanelSelector(this.patcher);
    if (selector && args.length > 0) {
        selector.select(args[0]);
    }
}

function input()
{
    selectPanel(this.patcher, "input");
}

function saturator()
{
    selectPanel(this.patcher, "saturator");
}

function compressor()
{
    selectPanel(this.patcher, "compressor");
}

function equalizer()
{
    selectPanel(this.patcher, "equalizer");
}

function output()
{
    selectPanel(this.patcher, "output");
}

function selectPanel(patcher, panel)
{
    let selector = ensurePanelSelector(patcher);
    if (selector) {
        selector.select(panel);
    }
}
