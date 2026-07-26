autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet 0: refresh; host.query <requesterId>; host.announce <deviceId> <label...>; host.leave <deviceId>.
// Outlet 0: host.query <requesterId>; host.announce <deviceId> <label...>; host.leave <deviceId>.

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var HostDirectoryVisualOptions = {
    backgroundColor: [0.08, 0.08, 0.08, 1.0],
    rowColor: [0.14, 0.14, 0.14, 1.0],
    currentRowColor: [0.10, 0.34, 0.40, 1.0],
    borderColor: [0.28, 0.28, 0.28, 1.0],
    textColor: [0.88, 0.88, 0.88, 1.0],
    mutedTextColor: [0.58, 0.58, 0.58, 1.0],
    accentColor: [0.10, 0.78, 0.92, 1.0],
    padding: 8,
    headerHeight: 22,
    rowHeight: 24,
    minimumTextSize: 8,
    maximumTextSize: 12
};

function HostEntry(id, label, current) {
    this.id = id;
    this.label = label;
    this.current = current;
}

function HostDirectory() {
    this.deviceId = 0;
    this.directEntries = [];
    this.entries = [];
    this.knownPeers = {};
    this.scrollOffset = 0;
}

HostDirectory.prototype.Initialize = function() {
    this.deviceId = this.CurrentDeviceId();
    this.Refresh();
    this.Announce();
    if (this.deviceId > 0) outlet(0, "host.query", this.deviceId);
};

HostDirectory.prototype.CurrentDeviceId = function() {
    try {
        return Number(new LiveAPI("this_device").id) || 0;
    } catch (error) {
        return 0;
    }
};

HostDirectory.prototype.Refresh = function() {
    var entries = [];
    var visited = {};
    this.CollectTrackEntries("live_set", "", entries, visited);
    this.directEntries = entries;
    this.RebuildEntries();
};

HostDirectory.prototype.RebuildEntries = function() {
    var entries = this.directEntries.slice();
    var knownIds = {};
    for (var index = 0; index < entries.length; index++) {
        knownIds[entries[index].id] = true;
    }
    for (var peerId in this.knownPeers) {
        if (!this.knownPeers.hasOwnProperty(peerId) || knownIds[peerId]) continue;
        entries.push(new HostEntry(Number(peerId), this.knownPeers[peerId], false));
    }
    this.entries = entries;
    this.ClampScroll();
    mgraphics.redraw();
};

HostDirectory.prototype.CollectTrackEntries = function(path, parentLabel, entries, visited) {
    var root = this.Api(path);
    if (!root) return;

    var trackIds = this.ReadIds(root, "tracks");
    for (var index = 0; index < trackIds.length; index++) {
        this.CollectTrack(trackIds[index], parentLabel, entries, visited);
    }

    var returnTrackIds = this.ReadIds(root, "return_tracks");
    for (var returnIndex = 0; returnIndex < returnTrackIds.length; returnIndex++) {
        this.CollectTrack(returnTrackIds[returnIndex], "Returns", entries, visited);
    }

    var masterId = this.ReadSingleId(root, "master_track");
    if (masterId > 0) this.CollectTrack(masterId, "Master", entries, visited);
};

HostDirectory.prototype.CollectTrack = function(trackId, parentLabel, entries, visited) {
    var track = this.Api("id " + trackId);
    if (!track) return;
    var trackName = this.ReadString(track, "name", "Track");
    var label = parentLabel ? parentLabel + " / " + trackName : trackName;
    this.CollectDevices(track, label, entries, visited);
};

HostDirectory.prototype.CollectDevices = function(owner, ownerLabel, entries, visited) {
    var deviceIds = this.ReadIds(owner, "devices");
    for (var index = 0; index < deviceIds.length; index++) {
        var deviceId = deviceIds[index];
        if (visited[deviceId]) continue;
        visited[deviceId] = true;

        var device = this.Api("id " + deviceId);
        if (!device) continue;
        var deviceName = this.ReadString(device, "name", "Device");
        if (this.IsConsolidator(deviceId, deviceName)) {
            entries.push(new HostEntry(deviceId, ownerLabel + " / " + deviceName, deviceId === this.deviceId));
        }
        this.CollectDeviceChains(device, ownerLabel + " / " + deviceName, entries, visited);
    }
};

HostDirectory.prototype.CollectDeviceChains = function(device, ownerLabel, entries, visited) {
    var chainIds = this.ReadIds(device, "chains");
    for (var index = 0; index < chainIds.length; index++) {
        var chain = this.Api("id " + chainIds[index]);
        if (!chain) continue;
        var chainName = this.ReadString(chain, "name", "Chain");
        this.CollectDevices(chain, ownerLabel + " / " + chainName, entries, visited);
    }
};

HostDirectory.prototype.IsConsolidator = function(deviceId, deviceName) {
    return deviceId === this.deviceId || /consolidator/i.test(deviceName);
};

HostDirectory.prototype.Api = function(path) {
    try {
        return new LiveAPI(path);
    } catch (error) {
        return null;
    }
};

HostDirectory.prototype.ReadIds = function(api, property) {
    try {
        var values = api.get(property);
        var ids = [];
        for (var index = 0; index < values.length; index++) {
            if (values[index] === "id") continue;
            var id = Number(values[index]);
            if (isFinite(id) && id > 0) ids.push(id);
        }
        return ids;
    } catch (error) {
        return [];
    }
};

HostDirectory.prototype.ReadSingleId = function(api, property) {
    var ids = this.ReadIds(api, property);
    return ids.length > 0 ? ids[0] : 0;
};

HostDirectory.prototype.ReadString = function(api, property, fallback) {
    try {
        var values = api.get(property);
        return values.length > 0 ? String(values[0]) : fallback;
    } catch (error) {
        return fallback;
    }
};

HostDirectory.prototype.HandleBusMessage = function(name, values) {
    if (name === "refresh") {
        this.Initialize();
        return;
    }
    if (name === "host.query") {
        if (Number(values[0]) !== this.deviceId) this.Announce();
        return;
    }
    if (name === "host.announce") {
        var peerId = Number(values[0]);
        if (!isFinite(peerId) || peerId <= 0 || peerId === this.deviceId || values.length < 2) return;
        this.knownPeers[peerId] = values.slice(1).join(" ");
        this.RebuildEntries();
        return;
    }
    if (name === "host.leave") {
        var departedId = Number(values[0]);
        delete this.knownPeers[departedId];
        var remaining = [];
        for (var index = 0; index < this.directEntries.length; index++) {
            if (this.directEntries[index].id !== departedId) remaining.push(this.directEntries[index]);
        }
        this.directEntries = remaining;
        this.RebuildEntries();
    }
};

HostDirectory.prototype.Announce = function() {
    var current = this.CurrentEntry();
    if (current) outlet(0, "host.announce", current.id, current.label);
};

HostDirectory.prototype.CurrentEntry = function() {
    for (var index = 0; index < this.entries.length; index++) {
        if (this.entries[index].current) return this.entries[index];
    }
    return null;
};

HostDirectory.prototype.VisibleRowCount = function() {
    var height = mgraphics.size[1];
    var available = height - HostDirectoryVisualOptions.padding * 2 - HostDirectoryVisualOptions.headerHeight;
    return Math.max(1, Math.floor(available / HostDirectoryVisualOptions.rowHeight));
};

HostDirectory.prototype.ClampScroll = function() {
    var maximum = Math.max(0, this.entries.length - this.VisibleRowCount());
    this.scrollOffset = Math.max(0, Math.min(this.scrollOffset, maximum));
};

HostDirectory.prototype.Scroll = function(delta) {
    if (!isFinite(delta) || delta === 0) return;
    this.scrollOffset += delta > 0 ? -1 : 1;
    this.ClampScroll();
    mgraphics.redraw();
};

HostDirectory.prototype.FitText = function(text, width, rowHeight) {
    var size = Math.min(HostDirectoryVisualOptions.maximumTextSize, rowHeight * 0.48);
    size = Math.max(HostDirectoryVisualOptions.minimumTextSize, size);
    mgraphics.set_font_size(size);
    var fitted = String(text);
    while (fitted.length > 1 && mgraphics.text_measure(fitted)[0] > width) {
        fitted = fitted.substring(0, fitted.length - 1);
    }
    return fitted === text ? fitted : fitted.substring(0, Math.max(1, fitted.length - 3)) + "...";
};

HostDirectory.prototype.DrawHeader = function(width) {
    var title = "PROJECT HOSTS";
    var count = String(this.entries.length);
    mgraphics.set_source_rgba(HostDirectoryVisualOptions.mutedTextColor);
    mgraphics.set_font_size(9);
    mgraphics.move_to(HostDirectoryVisualOptions.padding, 15);
    mgraphics.show_text(title);
    var countWidth = mgraphics.text_measure(count)[0];
    mgraphics.move_to(width - HostDirectoryVisualOptions.padding - countWidth, 15);
    mgraphics.show_text(count);
};

HostDirectory.prototype.DrawEntry = function(entry, row, width) {
    var options = HostDirectoryVisualOptions;
    var y = options.padding + options.headerHeight + row * options.rowHeight;
    mgraphics.set_source_rgba(entry.current ? options.currentRowColor : options.rowColor);
    mgraphics.rectangle(options.padding, y, width - options.padding * 2, options.rowHeight - 2);
    mgraphics.fill();

    if (entry.current) {
        mgraphics.set_source_rgba(options.accentColor);
        mgraphics.rectangle(options.padding, y, 2, options.rowHeight - 2);
        mgraphics.fill();
    }

    var label = this.FitText(entry.label, width - options.padding * 2 - 12, options.rowHeight);
    mgraphics.set_source_rgba(options.textColor);
    mgraphics.move_to(options.padding + 7, y + options.rowHeight * 0.64);
    mgraphics.show_text(label);
};

HostDirectory.prototype.Paint = function() {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var options = HostDirectoryVisualOptions;
    mgraphics.set_source_rgba(options.backgroundColor);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    mgraphics.set_source_rgba(options.borderColor);
    mgraphics.set_line_width(1);
    mgraphics.rectangle(0.5, 0.5, width - 1, height - 1);
    mgraphics.stroke();
    mgraphics.select_font_face("Ableton Sans", 0, 0);
    this.DrawHeader(width);
    var end = Math.min(this.entries.length, this.scrollOffset + this.VisibleRowCount());
    for (var index = this.scrollOffset; index < end; index++) {
        this.DrawEntry(this.entries[index], index - this.scrollOffset, width);
    }
};

var hostDirectory = new HostDirectory();

function inletassist(index) {
    assist(index === 0
        ? "refresh; host.query <requesterId>; host.announce <deviceId> <label...>; host.leave <deviceId>"
        : "");
}

function outletassist(index) {
    assist(index === 0
        ? "host.query <requesterId>; host.announce <deviceId> <label...>; host.leave <deviceId>"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() { hostDirectory.Initialize(); }
function refresh() { hostDirectory.Initialize(); }
function paint() { hostDirectory.Paint(); }
function onmousewheel(x, y, delta) { hostDirectory.Scroll(delta); }
function anything() { hostDirectory.HandleBusMessage(messagename, arrayfromargs(arguments)); }
function list() { hostDirectory.HandleBusMessage("list", arrayfromargs(arguments)); }
function notifydeleted() {
    if (hostDirectory.deviceId > 0) outlet(0, "host.leave", hostDirectory.deviceId);
}
