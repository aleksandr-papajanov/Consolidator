function BankManagerLiveIdentity() {
}

BankManagerLiveIdentity.prototype.Resolve = function() {
    try {
        var device = new LiveAPI("this_device");
        var liveObjectId = Number(device.id);
        if (!isFinite(liveObjectId) || liveObjectId <= 0) return null;
        var parent = device.get("canonical_parent");
        if (!parent || parent.length < 2) return null;
        var trackId = Number(parent[1]);
        if (!isFinite(trackId) || trackId <= 0) return null;
        var track = new LiveAPI("id " + trackId);
        if (!isFinite(Number(track.id)) || Number(track.id) <= 0) return null;
        var trackName = String(track.get("name")[0] || "");
        var trackOrder = this.TrackOrder(trackId);
        if (!trackName || !isFinite(trackOrder)) return null;
        return {
            id: "live-device-" + String(liveObjectId),
            trackId: trackId,
            trackName: trackName,
            trackOrder: trackOrder
        };
    } catch (error) {
        return null;
    }
};

BankManagerLiveIdentity.prototype.TrackOrder = function(trackId) {
    var liveSet = new LiveAPI("live_set");
    var tracksCount = Number(liveSet.getcount("tracks"));
    if (isFinite(tracksCount) && tracksCount >= 0) {
        for (var trackIndex = 0; trackIndex < tracksCount; ++trackIndex) {
            var track = new LiveAPI("live_set tracks " + trackIndex);
            if (Number(track.id) === trackId) return trackIndex;
        }
    }
    var returnCount = Number(liveSet.getcount("return_tracks"));
    if (isFinite(returnCount) && returnCount >= 0) {
        for (var returnIndex = 0; returnIndex < returnCount; ++returnIndex) {
            var returnTrack = new LiveAPI("live_set return_tracks " + returnIndex);
            if (Number(returnTrack.id) === trackId) {
                return (isFinite(tracksCount) ? tracksCount : 0) + returnIndex;
            }
        }
    }
    var masterTrack = new LiveAPI("live_set master_track");
    if (Number(masterTrack.id) === trackId) {
        return (isFinite(tracksCount) ? tracksCount : 0)
            + (isFinite(returnCount) ? returnCount : 0);
    }
    return NaN;
};
