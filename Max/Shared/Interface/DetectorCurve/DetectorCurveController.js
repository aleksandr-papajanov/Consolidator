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
            closest = { filterId: index + 1, filter: filter };
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
        this.EmitForFilter(match.filterId, "bypass", match.filter.bypass ? 1 : 0);
        if (match.filter.bypass) {
            this.model.SetListen(match.filterId, 0);
            outlet(0, "detector_listen", match.filterId, 0);
        }
        mgraphics.redraw();
        return;
    }
    if (option) {
        if (match.filter.bypass) return;
        var enabled = this.model.IsListening(match.filterId) ? 0 : 1;
        this.model.SetListen(match.filterId, enabled);
        outlet(0, "detector_listen", match.filterId, enabled);
        mgraphics.redraw();
        return;
    }
    var now = new Date().getTime();
    if (this.lastClickFilterId === match.filterId && now - this.lastClickTime <= 600) {
        this.lastClickFilterId = 0;
        this.lastClickTime = 0;
        this.dragFilterId = match.filterId;
        this.dragStart = null;
        this.Emit("reset", 1);
        mgraphics.redraw();
        return;
    }
    this.lastClickFilterId = match.filterId;
    this.lastClickTime = now;
    this.dragFilterId = match.filterId;
    this.dragStart = {
        x: Number(x),
        y: Number(y),
        frequencyHz: match.filter.frequencyHz,
        gainDb: match.filter.gainDb,
        q: match.filter.q,
        editQ: Boolean(option)
    };
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
        filter.q = this.renderer.Clamp(
            this.dragStart.q * Math.pow(8.0,
                (this.dragStart.y - Number(y)) / Math.max(1.0, plotHeight)),
            definition.qMinimum,
            definition.qMaximum
        );
        this.Emit("q", filter.q);
    } else {
        filter.frequencyHz = this.renderer.Clamp(
            this.renderer.PointToFrequency(x, mgraphics.size[0]),
            definition.frequencyMinimum,
            definition.frequencyMaximum
        );
        filter.gainDb = this.renderer.Clamp(
            this.renderer.PointToGain(y, plotHeight),
            definition.gainMinimum,
            definition.gainMaximum
        );
        this.Emit("frequency", filter.frequencyHz);
        this.Emit("gain", filter.gainDb);
    }
    mgraphics.redraw();
};

DetectorCurveController.prototype.EndDrag = function() {
    this.dragFilterId = 0;
    this.dragStart = null;
};

DetectorCurveController.prototype.Emit = function(parameter, value) {
    this.EmitForFilter(this.dragFilterId, parameter, value);
};

DetectorCurveController.prototype.EmitForFilter = function(
    filterId,
    parameter,
    value
) {
    outlet(0, "detector_absolute", Number(filterId), parameter, value);
};

DetectorCurveController.prototype.SetListen = function(filterId) {
    this.model.SetListen(filterId, arguments.length > 1 ? arguments[1] : 0);
    mgraphics.redraw();
};
