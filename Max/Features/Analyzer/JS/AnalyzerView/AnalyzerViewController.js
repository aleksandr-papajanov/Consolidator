function AnalyzerViewController() {
    this.state = new AnalyzerViewState();
    this.redrawPending = false;
    this.redrawTask = new Task(this.FlushRedraw, this);
}

AnalyzerViewController.prototype.RequestRedraw = function() {
    if (this.redrawPending) return;
    this.redrawPending = true;
    this.redrawTask.schedule(0);
};

AnalyzerViewController.prototype.FlushRedraw = function() {
    this.redrawPending = false;
    mgraphics.redraw();
};

AnalyzerViewController.prototype.Paint = function() {
    analyzerViewRenderer.Paint(this.state);
};

AnalyzerViewController.prototype.SetMode = function(mode) {
    if (mode !== "spectrum" && mode !== "analysis") return;
    if (this.state.mode === mode) return;
    this.state.mode = mode;
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ToggleScale = function() {
    var options = analyzerViewConfig.spectrum.scaleOptionsDb;
    this.state.scaleIndex = (this.state.scaleIndex + 1) % options.length;
    analyzerViewConfig.spectrum.scaleDb = options[this.state.scaleIndex];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearSpectrum = function() {
    this.state.currentCurve = [];
    this.state.referenceCurve = [];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearFitCurve = function() {
    this.state.fitCurve = [];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearAnalysis = function() {
    this.state.analysis = { metrics: [], bands: [], windowCount: 0, historySeconds: 0 };
    this.RequestRedraw();
};

AnalyzerViewController.prototype.SetCurve = function(kind, values) {
    if (this.state.curveSettings.pointCount > 0 && values.length !== this.state.curveSettings.pointCount) return;
    this.state[kind] = values;
    this.RequestRedraw();
};

AnalyzerViewController.prototype.HandleList = function(inletIndex, values) {
    if (inletIndex === 0) this.SetCurve("currentCurve", values);
    else if (inletIndex === 1) this.SetCurve("referenceCurve", values);
    else if (inletIndex === 2 && values.length > 1 && String(values[0]) === "fit_curve") this.SetCurve("fitCurve", values.slice(1));
    else if (inletIndex === 4) this.SetCurve("totalCurve", values);
    else if (inletIndex === 5) this.SetFeatureVector(values);
    else if (inletIndex === 6) this.HandleSnapshot(values);
};

AnalyzerViewController.prototype.HandleAnything = function(inletIndex, name, values) {
    if (inletIndex === 2 && name === "fit_curve") this.SetCurve("fitCurve", values);
    else if (inletIndex === 3 && name === "filter_curve") this.SetFilterCurve(values);
    else if (inletIndex === 3 && name === "curve_settings") this.SetCurveSettings(values);
    else if (inletIndex === 6 && name === "mode" && values.length === 1) this.SetMode(String(values[0]));
    else if (inletIndex === 6 && name === "snapshot") this.HandleSnapshot(["snapshot"].concat(values));
};

AnalyzerViewController.prototype.SetCurveSettings = function(values) {
    if (values.length !== 3) return;
    var minimumHz = Number(values[0]);
    var maximumHz = Number(values[1]);
    var pointCount = Number(values[2]);
    if (!isFinite(minimumHz) || !isFinite(maximumHz) || !isFinite(pointCount) || minimumHz <= 0 || maximumHz <= minimumHz || pointCount < 2) return;
    this.state.curveSettings = { minimumHz: minimumHz, maximumHz: maximumHz, pointCount: pointCount };
};

AnalyzerViewController.prototype.SetFilterCurve = function(values) {
    if (values.length < 13) return;
    var filterId = Number(values[0]);
    var active = Number(values[1]) !== 0;
    var curve = values.slice(12).map(Number);
    if (!isFinite(filterId) || (active && this.state.curveSettings.pointCount > 0 && curve.length !== this.state.curveSettings.pointCount)) return;
    var item = {
        filterId: filterId,
        active: active,
        frequency: Number(values[2]),
        gain: Number(values[3]),
        type: String(values[4]),
        q: Number(values[5]),
        qMinimum: Number(values[6]),
        qMaximum: Number(values[7]),
        frequencyMinimum: Number(values[8]),
        frequencyMaximum: Number(values[9]),
        gainMinimum: Number(values[10]),
        gainMaximum: Number(values[11])
    };
    this.UpsertHandle(item);
    if (active) {
        this.state.filterCurves[String(filterId)] = {
            curve: curve,
            color: analyzerViewConfig.spectrum.filter,
            type: item.type
        };
    }
    else delete this.state.filterCurves[String(filterId)];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.UpsertHandle = function(handle) {
    for (var index = 0; index < this.state.handles.length; ++index) {
        if (this.state.handles[index].filterId === handle.filterId) {
            this.state.handles[index] = handle;
            return;
        }
    }
    this.state.handles.push(handle);
};

AnalyzerViewController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot" || Number(values[1]) !== 1 || String(values[3]) !== "eq") return;
    var selectedBankId = Number(values[5]);
    if (isFinite(selectedBankId) && selectedBankId >= 1) this.state.selectedBankId = selectedBankId;
};

AnalyzerViewController.prototype.SetFeatureVector = function(values) {
    if (values.length < 3) return;
    var position = 0;
    var windowCount = Number(values[position++]);
    var historySeconds = Number(values[position++]);
    var metricCount = Number(values[position++]);
    if (!isFinite(windowCount) || !isFinite(historySeconds) || metricCount < 0) return;
    var metrics = [];
    for (var index = 0; index < metricCount; ++index) {
        if (position + 4 >= values.length) return;
        metrics.push({
            id: String(values[position++]),
            currentMean: Number(values[position++]),
            currentDeviation: Number(values[position++]),
            referenceMean: Number(values[position++]),
            referenceDeviation: Number(values[position++])
        });
    }
    if (position + 1 >= values.length) return;
    var bandCount = Number(values[position++]);
    var bandMetricCount = Number(values[position++]);
    var metricIds = [];
    for (var metricIndex = 0; metricIndex < bandMetricCount; ++metricIndex) {
        if (position >= values.length) return;
        metricIds.push(String(values[position++]));
    }
    var bands = [];
    for (var bandIndex = 0; bandIndex < bandCount; ++bandIndex) {
        if (position + 1 >= values.length) return;
        var band = { minimumHz: Number(values[position++]), maximumHz: Number(values[position++]), metrics: [] };
        for (metricIndex = 0; metricIndex < bandMetricCount; ++metricIndex) {
            if (position + 3 >= values.length) return;
            band.metrics.push({
                id: metricIds[metricIndex],
                current: Number(values[position++]),
                currentDeviation: Number(values[position++]),
                reference: Number(values[position++]),
                referenceDeviation: Number(values[position++])
            });
        }
        bands.push(band);
    }
    this.state.analysis = { windowCount: windowCount, historySeconds: historySeconds, metrics: metrics, bands: bands };
    this.RequestRedraw();
};

AnalyzerViewController.prototype.OnClick = function(x, y, button, cmd, shift, capslock, option) {
    if (this.HandleViewControl(x, y)) return;
    if (this.state.mode !== "spectrum") return;
    var handle = this.FindHandleAt(x, y);
    this.state.selectedHandleId = handle ? handle.filterId : null;
    this.state.dragHandleId = handle ? handle.filterId : null;
    if (!handle) return;
    this.state.dragStart = {
        x: x,
        y: y,
        frequency: handle.frequency,
        gain: handle.gain,
        q: handle.q,
        editQ: Boolean(option),
        qMinimum: handle.qMinimum,
        qMaximum: handle.qMaximum,
        frequencyMinimum: handle.frequencyMinimum,
        frequencyMaximum: handle.frequencyMaximum,
        gainMinimum: handle.gainMinimum,
        gainMaximum: handle.gainMaximum
    };
    this.RequestRedraw();
};

AnalyzerViewController.prototype.HandleViewControl = function(x, y) {
    var settings = analyzerViewConfig.spectrum;
    if (y > settings.controlHeight) return false;
    if (x >= settings.controlPadding && x <= settings.controlPadding + 48) {
        outlet(0, "view_mode", "spectrum");
        this.SetMode("spectrum");
        return true;
    }
    if (x >= settings.controlPadding + 50 && x <= settings.controlPadding + 114) {
        outlet(0, "view_mode", "analysis");
        this.SetMode("analysis");
        return true;
    }
    var size = mgraphics.size;
    if (x >= size[0] - 50) {
        this.ToggleScale();
        return true;
    }
    return false;
};

AnalyzerViewController.prototype.OnDrag = function(x, y, button, cmd, shift, capslock, option) {
    if (!button || this.state.dragHandleId === null || !this.state.dragStart) return;
    var handle = this.FindHandle(this.state.dragHandleId);
    if (!handle) return;
    var size = mgraphics.size;
    var bottom = analyzerViewGeometry.PlotBottom(size[1]);
    var start = this.state.dragStart;
    if (Boolean(option) && start.qMaximum > start.qMinimum) {
        var qRatio = analyzerViewGeometry.Clamp((start.y - y) / bottom, -1, 1);
        handle.q = analyzerViewGeometry.Clamp(start.q * Math.pow(8, qRatio), start.qMinimum, start.qMaximum);
        this.SendParameter(handle.filterId, "q", handle.q);
    }
    else {
        handle.frequency = analyzerViewGeometry.Clamp(
            start.frequency * Math.pow(start.frequencyMaximum / start.frequencyMinimum, (x - start.x) / Math.max(1, size[0])),
            start.frequencyMinimum,
            start.frequencyMaximum);
        handle.gain = analyzerViewGeometry.Clamp(
            start.gain + (start.y - y) / Math.max(1, bottom - analyzerViewGeometry.PlotTop()) * analyzerViewConfig.spectrum.scaleDb * 2,
            start.gainMinimum,
            start.gainMaximum
        );
        if (handle.type === "gain") this.SendParameter(handle.filterId, "gain", handle.gain);
        else {
            this.SendParameter(handle.filterId, handle.type === "tilt" ? "pivot" : "freq", handle.frequency);
            this.SendParameter(handle.filterId, "gain", handle.gain);
        }
    }
    this.RequestRedraw();
};

AnalyzerViewController.prototype.OnMouseUp = function() {
    this.state.dragHandleId = null;
    this.state.dragStart = null;
};

AnalyzerViewController.prototype.FindHandle = function(filterId) {
    for (var index = 0; index < this.state.handles.length; ++index) {
        if (this.state.handles[index].filterId === filterId && this.state.handles[index].active) return this.state.handles[index];
    }
    return null;
};

AnalyzerViewController.prototype.FindHandleAt = function(x, y) {
    var size = mgraphics.size;
    var bottom = analyzerViewGeometry.PlotBottom(size[1]);
    var radius = analyzerViewConfig.spectrum.handleHitRadius;
    var closest = null;
    var closestDistance = radius * radius;
    for (var index = 0; index < this.state.handles.length; ++index) {
        var handle = this.state.handles[index];
        if (!handle.active) continue;
        var handleX = handle.type === "gain"
            ? analyzerViewConfig.spectrum.gainHandleX
            : analyzerViewGeometry.FrequencyToX(handle.frequency, size[0]);
        var dx = x - handleX;
        var dy = y - analyzerViewGeometry.DbToY(handle.gain, bottom);
        var distance = dx * dx + dy * dy;
        if (distance <= closestDistance) {
            closest = handle;
            closestDistance = distance;
        }
    }
    return closest;
};

AnalyzerViewController.prototype.SendParameter = function(filterId, parameter, value) {
    this.state.requestId += 1;
    outlet(0, "command", 1, "spectrum", this.state.requestId, "eq.set_parameter",
        this.state.selectedBankId, filterId, parameter, value);
};
