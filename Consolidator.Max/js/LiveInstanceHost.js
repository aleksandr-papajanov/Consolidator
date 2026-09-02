inlets = 1;
outlets = 1;

class LiveInstanceHost
{
    constructor(LiveAPIConstructor, emit)
    {
        this.LiveAPI = LiveAPIConstructor;
        this.emit = emit || (() => {});
        this.trackObserver = null;
        this.selectedTrackObserver = null;
        this.deviceId = 0;
        this.trackId = 0;
        this.selectedTrackId = 0;
        this.selectionMonitoringEnabled = false;
        this.lastPublishedActive = null;
    }

    normalizeName(value)
    {
        let name = String(value || "");
        if (name.length >= 2 && name.charAt(0) === '"' &&
                name.charAt(name.length - 1) === '"')
        {
            return name.substring(1, name.length - 1);
        }
        return name;
    }

    logTrackName(message)
    {
        if (typeof post === "function")
        {
            post("[Consolidator][TrackName] LiveInstanceHost " + message + "\n");
        }
    }

    readId(values)
    {
        if (!values)
        {
            return 0;
        }
        let items = values instanceof Array ? values : [values];
        for (let index = 0; index < items.length - 1; index += 1)
        {
            if (String(items[index]) === "id")
            {
                let value = Number(items[index + 1]);
                return isFinite(value) && value > 0 ? value : 0;
            }
        }
        return 0;
    }

    publishName(value)
    {
        let name = this.normalizeName(value);
        this.logTrackName("publish raw=" + JSON.stringify(value) +
            " normalized=" + JSON.stringify(name));
        this.emit(["track_name", name]);
    }

    publishActivity()
    {
        let active = this.deviceId > 0 && this.trackId > 0 &&
            (!this.selectionMonitoringEnabled ||
                this.selectedTrackId === this.trackId);
        if (this.lastPublishedActive === active)
        {
            return;
        }
        this.lastPublishedActive = active;
        this.emit(["instance_active", active ? 1 : 0]);
    }

    trackNameChanged(values)
    {
        this.logTrackName("callback=" + JSON.stringify(values));
        if (!values || values.length < 2 || String(values[0]) !== "name")
        {
            this.logTrackName("ignored callback");
            return;
        }
        this.publishName(values[1]);
    }

    selectedTrackChanged(values)
    {
        if (!values || values.length < 2 ||
                String(values[0]) !== "selected_track")
        {
            return;
        }
        this.selectedTrackId = this.readId(values);
        this.publishActivity();
    }

    bang()
    {
        let device = new this.LiveAPI(null, "this_device");
        this.deviceId = Number(device.id);
        let track = new this.LiveAPI(null, "this_device canonical_parent");
        this.trackId = Number(track.id);
        this.logTrackName("bang deviceId=" + this.deviceId +
            " trackId=" + this.trackId +
            " trackPath=" + JSON.stringify(track.unquotedpath || ""));
        if (!isFinite(this.deviceId) || this.deviceId <= 0 || this.trackId <= 0)
        {
            this.logTrackName("activation stopped: invalid ids");
            return;
        }

        let name = track.get("name");
        this.logTrackName("initial get(name)=" + JSON.stringify(name));
        this.publishName(name && name.length ? name[0] : "");
        let trackPath = String(track.unquotedpath || "");
        if (!trackPath)
        {
            this.logTrackName("activation stopped: empty track path");
            return;
        }
        this.trackObserver = new this.LiveAPI((values) =>
        {
            this.trackNameChanged(values);
        }, "id " + this.trackId);
        this.trackObserver.property = "name";

        this.selectedTrackObserver = new this.LiveAPI((values) =>
        {
            this.selectedTrackChanged(values);
        }, "live_set view");
        this.selectedTrackObserver.property = "selected_track";

        this.selectedTrackId = this.readId(
            this.selectedTrackObserver.get("selected_track"));
        this.publishActivity();
        this.selectionMonitoringEnabled = true;
    }

    destroy()
    {
        this.trackObserver = null;
        this.selectedTrackObserver = null;
    }
}

let liveInstanceHost = null;

function ensureHost()
{
    if (!liveInstanceHost)
    {
        liveInstanceHost = new LiveInstanceHost(
            LiveAPI,
            (message) =>
            {
                outlet(0, message);
            }
        );
    }
    return liveInstanceHost;
}

function bang()
{
    ensureHost().bang();
}

function notifydeleted()
{
    if (liveInstanceHost)
    {
        liveInstanceHost.destroy();
        liveInstanceHost = null;
    }
}
