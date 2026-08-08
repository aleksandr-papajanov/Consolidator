include("DetectorCurveOptions.js");

function DetectorCurveController(model, renderer) {
    this.model = model;
    this.renderer = renderer;
    this.dragFilterId = 0;
    this.dragStart = null;
    this.lastClickFilterId = 0;
    this.lastClickTime = 0;
}

DetectorCurveController.prototype.PlotHeight = function() {
    var size = mgraphics.size;
    return Math.max(
        1.0,
        size[1] - detectorCurveOptions.labelHeight - detectorCurveOptions.padding * 2.0
    );
};

DetectorCurveController.prototype.FindFilterAt = function(x, y) {
    var closest = null;
    var closestDistance = detectorCurveOptions.markerHitRadius
        * detectorCurveOptions.markerHitRadius;
    var plotHeight = this.PlotHeight();
    for (var index = 0; index < this.model.filters.length; ++index) {
        var filter = this.model.filters[index];
        if (!filter || !filter.definition || filter.frequencyHz === null ||
            filter.gainDb === null) continue;
        var markerX = this.renderer.FrequencyToX(filter.frequencyHz, mgraphics.size[0]);
        var markerY = this.renderer.DbToY(filter.gainDb, plotHeight);
        var dx = Number(x) - markerX;
        var dy = Number(y) - markerY;
        var distance = dx * dx + dy * dy;
        if (distance <= closestDistance) {
            closest = { FilterId: index + 1, filter: filter };
            closestDistance = distance;
        }
    }
    return closest;
};

DetectorCurveController.prototype.BeginDrag = function(x, y, option, control) {
    var match = this.FindFilterAt(x, y);
    if (!match) return;
    if (control) {
        match.filter.bypass = !match.filter.bypass;
        this.EmitForFilter(match.FilterId, "bypass", match.filter.bypass ? 1 : 0);
        if (match.filter.bypass) {
            this.model.SetListen(match.FilterId, 0);
            outlet(0, "detector_listen", match.FilterId, 0);
        }
        mgraphics.redraw();
        return;
    }
    if (option) {
        if (match.filter.bypass) return;
        var enabled = this.model.IsListening(match.FilterId) ? 0 : 1;
        this.model.SetListen(match.FilterId, enabled);
        outlet(0, "detector_listen", match.FilterId, enabled);
        mgraphics.redraw();
        return;
    }
    var now = new Date().getTime();
    if (this.lastClickFilterId === match.FilterId && now - this.lastClickTime <= 600) {
        this.lastClickFilterId = 0;
        this.lastClickTime = 0;
        this.dragFilterId = match.FilterId;
        this.dragStart = null;
        this.Emit("reset", 1);
        mgraphics.redraw();
        return;
    }
    this.lastClickFilterId = match.FilterId;
    this.lastClickTime = now;
    this.dragFilterId = match.FilterId;
    this.dragStart = {
        x: Number(x),
        y: Number(y),
        frequencyHz: match.filter.frequencyHz,
        gainDb: match.filter.gainDb,
        q: match.filter.q,
        editQ: Boolean(option)
    };
    outlet(0, "gesture", "begin");
};

DetectorCurveController.prototype.Drag = function(x, y, button, option) {
    if (!button || !this.dragStart || this.dragFilterId < 1) return;
    var filter = this.model.filters[this.dragFilterId - 1];
    if (!filter) return;
    var options = detectorCurveOptions;
    var definition = filter.definition;
    if (!definition) return;
    var plotHeight = this.PlotHeight();
    if (Boolean(option)) {
        var qLimit = filter.Limit("q", definition.qMinimum, definition.qMaximum);
        filter.q = this.renderer.Clamp(
            this.dragStart.q * Math.pow(8.0,
                (this.dragStart.y - Number(y)) / Math.max(1.0, plotHeight)),
            qLimit.minimum,
            qLimit.maximum
        );
        this.Emit("q", filter.q);
    } else {
        var frequencyLimit = filter.Limit(
            "frequency", definition.frequencyMinimum, definition.frequencyMaximum);
        var gainLimit = filter.Limit("gain", definition.gainMinimum, definition.gainMaximum);
        filter.frequencyHz = this.renderer.Clamp(
            this.renderer.PointToFrequency(x, mgraphics.size[0]),
            frequencyLimit.minimum,
            frequencyLimit.maximum
        );
        filter.gainDb = this.renderer.Clamp(
            this.renderer.PointToGain(y, plotHeight),
            gainLimit.minimum,
            gainLimit.maximum
        );
        this.Emit("frequency", filter.frequencyHz);
        this.Emit("gain", filter.gainDb);
    }
    mgraphics.redraw();
};

DetectorCurveController.prototype.EndDrag = function() {
    if (this.dragFilterId > 0 && this.dragStart) outlet(0, "gesture", "end");
    this.dragFilterId = 0;
    this.dragStart = null;
};

DetectorCurveController.prototype.Emit = function(parameter, value) {
    this.EmitForFilter(this.dragFilterId, parameter, value);
};

DetectorCurveController.prototype.EmitForFilter = function(
    FilterId,
    parameter,
    value
) {
    outlet(0, "detector_absolute", Number(FilterId), parameter, value);
};

DetectorCurveController.prototype.SetListen = function(FilterId) {
    this.model.SetListen(FilterId, arguments.length > 1 ? arguments[1] : 0);
    mgraphics.redraw();
};
