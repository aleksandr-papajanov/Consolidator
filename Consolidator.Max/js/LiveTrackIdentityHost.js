inlets = 1;
outlets = 1;

var trackObserver = null;

function normalizeName(value) {
    var name = String(value || "");
    if (name.length >= 2 && name.charAt(0) === '"' &&
            name.charAt(name.length - 1) === '"') {
        return name.substring(1, name.length - 1);
    }
    return name;
}

function publishName(value) {
    var name = normalizeName(value);
    outlet(0, ["track_name", name]);
}

function trackNameChanged(values) {
    if (!values || values.length < 2 || String(values[0]) !== "name") {
        return;
    }
    publishName(values[1]);
}

function bang() {
    var device = new LiveAPI("this_device");
    var parent = device.get("canonical_parent");
    if (!parent || parent.length < 2) {
        return;
    }

    var trackId = Number(parent[1]);
    if (!isFinite(trackId) || trackId <= 0) {
        return;
    }

    var track = new LiveAPI("id " + trackId);
    var name = track.get("name");
    publishName(name && name.length ? name[0] : "");

    trackObserver = new LiveAPI(trackNameChanged, "id " + trackId);
    trackObserver.property = "name";
}

function freebang() {
    trackObserver = null;
}
