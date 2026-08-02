autowatch = 1;
outlets = 2;

include("../../Shared/Runtime/LiveApiInitializer.js");

function CoordinatorIdentity() {
    this.runtimeId = "";
    this.trackId = 0;
    this.trackName = "";
    this.trackOrder = -1;
    this.hostReady = false;
    this.trackNameObserver = null;
    this.initializer = new LiveApiInitializer(this.TryInitialize, this, 50);
}

CoordinatorIdentity.prototype.TryInitialize = function() {
    var identity = this.Resolve();
    if (!identity) return false;
    this.runtimeId = identity.runtimeId;
    this.trackId = identity.trackId;
    this.trackName = identity.trackName;
    this.trackOrder = identity.trackOrder;
    this.ObserveTrackName();
    this.Publish();
    return true;
};

CoordinatorIdentity.prototype.Resolve = function() {
    try {
        var device = new LiveAPI("this_device");
        var deviceId = Number(device.id);
        if (!isFinite(deviceId) || deviceId <= 0) return null;
        var parent = device.get("canonical_parent");
        if (!parent || parent.length < 2) return null;
        var trackId = Number(parent[1]);
        if (!isFinite(trackId) || trackId <= 0) return null;
        var track = new LiveAPI("id " + trackId);
        var trackName = String(track.get("name")[0] || "");
        var trackOrder = this.ResolveTrackOrder(trackId);
        if (!trackName || !isFinite(trackOrder)) return null;
        return {
            runtimeId: "live-device-" + String(deviceId),
            trackId: trackId,
            trackName: trackName,
            trackOrder: trackOrder
        };
    } catch (error) {
        return null;
    }
};

CoordinatorIdentity.prototype.ResolveTrackOrder = function(trackId) {
    var liveSet = new LiveAPI("live_set");
    var trackCount = Number(liveSet.getcount("tracks"));
    for (var index = 0; index < trackCount; ++index) {
        var track = new LiveAPI("live_set tracks " + index);
        if (Number(track.id) === trackId) return index;
    }
    var returnCount = Number(liveSet.getcount("return_tracks"));
    for (var returnIndex = 0; returnIndex < returnCount; ++returnIndex) {
        var returnTrack = new LiveAPI("live_set return_tracks " + returnIndex);
        if (Number(returnTrack.id) === trackId) return trackCount + returnIndex;
    }
    var master = new LiveAPI("live_set master_track");
    return Number(master.id) === trackId ? trackCount + returnCount : NaN;
};

CoordinatorIdentity.prototype.ObserveTrackName = function() {
    this.trackNameObserver = new LiveAPI(CoordinatorTrackNameChanged,
        "id " + this.trackId);
    this.trackNameObserver.property = "name";
};

CoordinatorIdentity.prototype.HandleTrackNameChanged = function(values) {
    if (values.length !== 2 || String(values[0]) !== "name") return;
    var trackName = String(values[1] || "");
    if (!trackName || trackName === this.trackName) return;
    this.trackName = trackName;
    this.Publish();
};

CoordinatorIdentity.prototype.Publish = function() {
    if (!this.runtimeId) return;
    outlet(0, "coordinator_identity", this.runtimeId,
        this.trackName, this.trackOrder);
    this.AnnounceChanged();
};

CoordinatorIdentity.prototype.AnnounceChanged = function() {
    if (!this.hostReady) return;
    outlet(1, "coordinator.changed");
};

CoordinatorIdentity.prototype.HandleHostEvent = function(values) {
    if (this.hostReady || values.indexOf("host.initialized") < 0) return;
    this.hostReady = true;
    this.AnnounceChanged();
};

CoordinatorIdentity.prototype.Dispose = function() {
    this.initializer.Dispose();
    if (this.runtimeId) outlet(0, "coordinator_remove", this.runtimeId);
    outlet(1, "coordinator.changed");
};

var coordinatorIdentity = new CoordinatorIdentity();

function CoordinatorTrackNameChanged() {
    coordinatorIdentity.HandleTrackNameChanged(arrayfromargs(arguments));
}

function loadbang() { coordinatorIdentity.initializer.Start(); }
function event() { coordinatorIdentity.HandleHostEvent(arrayfromargs(arguments)); }
function list() {
    var values = arrayfromargs(arguments);
    if (String(values[0]) === "event") values.shift();
    coordinatorIdentity.HandleHostEvent(values);
}
function notifydeleted() { coordinatorIdentity.Dispose(); }
