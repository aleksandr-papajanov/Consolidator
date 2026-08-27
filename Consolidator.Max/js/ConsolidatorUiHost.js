inlets = 2;
outlets = 2;

const { ConsolidatorUiHost, ConsolidatorControlMapping } = require("./ConsolidatorUiApplication.js");

let uiHost = null;

function sendNative(frame) {
    outlet(0, frame);
}

function sendUi(frame) {
    outlet(1, frame);
}

function ensureHost() {
    if (!uiHost) {
        let source = jsarguments.length > 1
            ? String(jsarguments[1]) : "consolidator.ui";
        uiHost = new ConsolidatorUiHost(source, sendNative, sendUi);
    }
    return uiHost;
}

function loadbang() {
    initializeHost();
}

function initializeHost() {
    ensureHost().initialize(ConsolidatorControlMapping);
}

function handleListMessage(inletIndex, args) {
    if (args.length < 2 || Number(args.shift()) !== 0) {
        return;
    }
    let selector = String(args.shift());
    if (inletIndex === 0 && selector === "track_name") {
        ensureHost().setTrackName(args);
        return;
    }
    if (inletIndex === 1) {
        if (args.length === 0) {
            return;
        }
        ensureHost().handleUiIntent(selector, String(args.shift()), args);
        return;
    }
    ensureHost().handleControl(selector, args);
}

function list(...args) {
    handleListMessage(this.inlet, args);
}

function live_ready() {
    initializeHost();
}

function undo() {
    ensureHost().undo();
}

function redo() {
    ensureHost().redo();
}

function destroy() {
    if (uiHost) {
        uiHost.destroy();
        uiHost = null;
    }
}

function notifydeleted() {
    destroy();
}
