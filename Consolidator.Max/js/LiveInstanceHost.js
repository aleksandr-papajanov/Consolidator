inlets = 1;
outlets = 1;

var trackObserver = null;
var selectedTrackObserver = null;
var selectedDeviceObserver = null;
var deviceId = 0;
var trackId = 0;
var selectedTrackId = 0;
var selectedDeviceId = 0;
var lastPublishedActive = null;

function normalizeName(value)
{
    var name = String(value || "");
    if (name.length >= 2 && name.charAt(0) === '"' &&
            name.charAt(name.length - 1) === '"')
    {
        return name.substring(1, name.length - 1);
    }
    return name;
}

function readId(values)
{
    if (!values)
    {
        return 0;
    }
    var items = values instanceof Array ? values : [values];
    for (var index = 0; index < items.length - 1; index += 1)
    {
        if (String(items[index]) === "id")
        {
            var value = Number(items[index + 1]);
            return isFinite(value) && value > 0 ? value : 0;
        }
    }
    return 0;
}

function publishName(value)
{
    outlet(0, ["track_name", normalizeName(value)]);
}

function publishActivity()
{
    var active = deviceId > 0 && trackId > 0 &&
        selectedTrackId === trackId && selectedDeviceId === deviceId;
    if (lastPublishedActive === active)
    {
        return;
    }
    lastPublishedActive = active;
    outlet(0, ["instance_active", active ? 1 : 0]);
}

function trackNameChanged(values)
{
    if (!values || values.length < 2 || String(values[0]) !== "name")
    {
        return;
    }
    publishName(values[1]);
}

function selectedTrackChanged(values)
{
    selectedTrackId = readId(values);
    publishActivity();
}

function selectedDeviceChanged(values)
{
    selectedDeviceId = readId(values);
    publishActivity();
}

function bang()
{
    var device = new LiveAPI("this_device");
    deviceId = Number(device.id);
    var parent = device.get("canonical_parent");
    trackId = readId(parent);
    if (!isFinite(deviceId) || deviceId <= 0 || trackId <= 0)
    {
        return;
    }

    var track = new LiveAPI("id " + trackId);
    var name = track.get("name");
    publishName(name && name.length ? name[0] : "");
    var trackPath = String(track.unquotedpath || "");
    if (!trackPath)
    {
        return;
    }
    var trackViewPath = trackPath + " view";

    trackObserver = new LiveAPI(trackNameChanged, "id " + trackId);
    trackObserver.property = "name";

    selectedTrackObserver = new LiveAPI(
        selectedTrackChanged,
        "live_set view");
    selectedTrackId = readId(
        selectedTrackObserver.get("selected_track"));
    selectedTrackObserver.property = "selected_track";

    selectedDeviceObserver = new LiveAPI(
        selectedDeviceChanged,
        trackViewPath);
    selectedDeviceId = readId(
        selectedDeviceObserver.get("selected_device"));
    selectedDeviceObserver.property = "selected_device";
    publishActivity();
}

function freebang()
{
    trackObserver = null;
    selectedTrackObserver = null;
    selectedDeviceObserver = null;
}
