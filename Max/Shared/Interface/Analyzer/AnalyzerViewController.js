include("../../Configuration/InterfaceTheme.js");
include("../Button/ButtonViewModel.js");
include("../ButtonGroup/ButtonGroupViewModel.js");
include("../ButtonGroup/ButtonGroupRenderer.js");

function AnalyzerColor(value) {
    return { r: value[0], g: value[1], b: value[2], a: value[3] };
}

var analyzerControlsOptions = {
    controlHeight: 18,
    controlPadding: InterfaceTheme.geometry.minimumPadding,
    modeGroupRatio: 0.28,
    groupGap: InterfaceTheme.geometry.valueGap
};

var analyzerUtilityButtonCount = 7;
var AnalyzerButtonGroupOptions = {
    content: CreateButtonGroupOptions({
        layout: "horizontal",
        selectionMode: "custom",
        sizing: "content"
    })
};
var analyzerButtonGroupRenderer = new ButtonGroupRenderer();

function AnalyzerUtilityLabels(scaleDb) {
    return [String(scaleDb) + " dB", "B", "R", "JOIN", "COMMIT", "MATCH EQ", "CLEAR"];
}

function CreateAnalyzerButtonGroup(labels) {
    return {
        buttons: [],
        labels: labels,
        loadingIndex: 0,
        enabled: true,
        pressedIndex: -1,
        visualStates: null,
        options: AnalyzerButtonGroupOptions.content,
        viewModel: new ButtonGroupViewModel(),
        cells: null
    };
}

var analyzerControlGroups = {
    mode: CreateAnalyzerButtonGroup(["FFT", "ANALYSIS"]),
    utility: CreateAnalyzerButtonGroup(AnalyzerUtilityLabels(24))
};
for (var analyzerModeIndex = 0; analyzerModeIndex < 2; ++analyzerModeIndex) {
    analyzerControlGroups.mode.buttons.push(new ButtonViewModel("toggle"));
}
var analyzerUtilityModes = ["momentary", "toggle", "momentary", "momentary", "momentary", "momentary", "momentary"];
for (var analyzerUtilityIndex = 0; analyzerUtilityIndex < analyzerUtilityModes.length; ++analyzerUtilityIndex) {
    analyzerControlGroups.utility.buttons.push(new ButtonViewModel(analyzerUtilityModes[analyzerUtilityIndex]));
}

function ResolveAnalyzerScaleDb(state) {
    return state.scaleDb !== undefined
        ? state.scaleDb
        : spectrumOptions.scaleOptionsDb[state.scaleIndex] || spectrumOptions.scaleDb;
}

function AnalyzerUtilityGroupRect(width) {
    var height = analyzerControlsOptions.controlHeight
        - analyzerControlsOptions.controlPadding * 2;
    var modeWidth = width * analyzerControlsOptions.modeGroupRatio;
    return {
        x: Math.min(width, modeWidth + analyzerControlsOptions.groupGap),
        y: analyzerControlsOptions.controlPadding,
        width: Math.max(0, width - modeWidth - analyzerControlsOptions.groupGap),
        height: height
    };
}

function AnalyzerModeGroupRect(width) {
    var height = analyzerControlsOptions.controlHeight
        - analyzerControlsOptions.controlPadding * 2;
    return {
        x: analyzerControlsOptions.controlPadding,
        y: analyzerControlsOptions.controlPadding,
        width: Math.max(0, width * analyzerControlsOptions.modeGroupRatio),
        height: height
    };
}

function AnalyzerUtilityGroupIndexAt(x, y, width, scaleDb) {
    return analyzerButtonGroupRenderer.IndexAt(analyzerControlGroups.utility, x, y);
}

function AnalyzerModeGroupIndexAt(x, y, width) {
    return analyzerButtonGroupRenderer.IndexAt(analyzerControlGroups.mode, x, y);
}

function AnalyzerControlsRenderer() {
    this.modeGroup = analyzerControlGroups.mode;
    this.utilityGroup = analyzerControlGroups.utility;
    this.buttonGroupRenderer = analyzerButtonGroupRenderer;
}

AnalyzerControlsRenderer.prototype.Paint = function(state, width) {
    var settings = analyzerControlsOptions;
    this.modeGroup.buttons[0].SetValue(state.mode === "spectrum" ? 1 : 0);
    this.modeGroup.buttons[1].SetValue(state.mode === "analysis" ? 1 : 0);
    this.buttonGroupRenderer.Paint(
        this.modeGroup,
        this.buttonGroupRenderer.Cells(
            this.modeGroup,
            AnalyzerModeGroupRect(width)
        )
    );
    var scaleDb = ResolveAnalyzerScaleDb(state);
    this.utilityGroup.buttons[0].SetValue(0);
    this.utilityGroup.buttons[1].SetValue(state.eqBypass ? 1 : 0);
    this.utilityGroup.buttons[2].SetValue(0);
    this.utilityGroup.buttons[3].SetValue(0);
    this.utilityGroup.buttons[4].SetValue(0);
    this.utilityGroup.buttons[5].SetValue(0);
    this.utilityGroup.buttons[6].SetValue(0);
    this.utilityGroup.labels = AnalyzerUtilityLabels(scaleDb);
    var operationAvailability = state.operationAvailability;
    this.utilityGroup.enabled = [
        true,
        operationAvailability.bypass,
        operationAvailability.reset,
        operationAvailability.join,
        operationAvailability.commit,
        operationAvailability.match,
        operationAvailability.clear
    ];
    this.buttonGroupRenderer.Paint(
        this.utilityGroup,
        this.buttonGroupRenderer.Cells(
            this.utilityGroup,
            AnalyzerUtilityGroupRect(width)
        )
    );
};

include("Spectrum/SpectrumOptions.js");
include("Spectrum/SpectrumGeometry.js");
include("Analysis/AnalysisOptions.js");
include("../../Configuration/FilterDefinitions.js");
include("AnalyzerViewModel.js");
include("AnalyzerViewRenderer.js");

function AnalyzerViewController() {
    this.state = {
        mode: "spectrum",
        scaleIndex: 1,
        currentCurve: [],
        referenceCurve: [],
        fitCurve: [],
        totalCurve: [],
        filterCurves: {},
        filterLimits: {},
        handles: [],
        selectedBankId: 1,
        eqBypass: false,
        hasEqSnapshot: false,
        operationAvailability: {
            bypass: false,
            reset: false,
            join: false,
            commit: false,
            match: false,
            clear: false
        },
        selectedHandleId: null,
        dragHandleId: null,
        dragStart: null,
        curveSettings: { minimumHz: 20, maximumHz: 20000, pointCount: 0 },
        analysis: { metrics: [], bands: [], windowCount: 0, historySeconds: 0 },
        requestId: 0
    };
    this.viewModel = new AnalyzerViewModel();
    this.redrawPending = false;
    this.redrawTask = new Task(this.FlushRedraw, this);
    this.lastClickFilterId = null;
    this.lastClickTime = 0;
    this.lastResetTime = 0;
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
    analyzerViewRenderer.Paint(this.viewModel.Build(this.state));
};

AnalyzerViewController.prototype.SetMode = function(mode) {
    if (mode !== "spectrum" && mode !== "analysis") return;
    if (this.state.mode === mode) return;
    this.state.mode = mode;
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ToggleScale = function() {
    var options = spectrumOptions.scaleOptionsDb;
    this.state.scaleIndex = (this.state.scaleIndex + 1) % options.length;
    spectrumOptions.scaleDb = options[this.state.scaleIndex];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearSpectrum = function() {
    var changed = this.state.currentCurve.length > 0 || this.state.referenceCurve.length > 0;
    this.state.currentCurve = [];
    this.state.referenceCurve = [];
    if (changed) this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearCurrentCurve = function() {
    if (this.state.currentCurve.length === 0) return;
    this.state.currentCurve = [];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearReferenceCurve = function() {
    if (this.state.referenceCurve.length === 0) return;
    this.state.referenceCurve = [];
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearFitCurve = function() {
    if (this.state.fitCurve.length === 0 &&
        !this.state.operationAvailability.match &&
        !this.state.operationAvailability.clear) return;
    this.state.fitCurve = [];
    this.state.operationAvailability.match = false;
    this.state.operationAvailability.clear = false;
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ClearAnalysis = function() {
    this.state.analysis = { metrics: [], bands: [], windowCount: 0, historySeconds: 0 };
    this.RequestRedraw();
};

AnalyzerViewController.prototype.SetCurve = function(kind, values) {
    if (this.state.curveSettings.pointCount > 0 && values.length !== this.state.curveSettings.pointCount) return;
    this.state[kind] = values;
    if (kind === "fitCurve") {
        this.state.operationAvailability.match = values.length > 1;
        this.state.operationAvailability.clear = values.length > 0;
    }
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
    else if (inletIndex === 6 && name === "eq_preview") this.SetEqPreview(values);
    else if (inletIndex === 6 && name === "filter_limits") this.SetFilterLimits(values);
};

AnalyzerViewController.prototype.SetCurveSettings = function(values) {
    if (values.length !== 5) return;
    var minimumHz = Number(values[0]);
    var maximumHz = Number(values[1]);
    var pointCount = Number(values[2]);
    var minimumSpectrumDb = Number(values[3]);
    var maximumSpectrumDb = Number(values[4]);
    if (!isFinite(minimumHz) || !isFinite(maximumHz) || !isFinite(pointCount) || minimumHz <= 0 || maximumHz <= minimumHz || pointCount < 2) return;
    if (!isFinite(minimumSpectrumDb) || !isFinite(maximumSpectrumDb) ||
        maximumSpectrumDb <= minimumSpectrumDb) return;
    this.state.curveSettings = { minimumHz: minimumHz, maximumHz: maximumHz, pointCount: pointCount };
    spectrumOptions.minimumFrequencyHz = minimumHz;
    spectrumOptions.maximumFrequencyHz = maximumHz;
    spectrumOptions.signalMinimumDb = minimumSpectrumDb;
    spectrumOptions.signalMaximumDb = maximumSpectrumDb;
};

AnalyzerViewController.prototype.SetFilterCurve = function(values) {
    if (values.length < 12) return;
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
    this.state.filterCurves[String(filterId)] = {
        curve: curve,
        color: spectrumOptions.filter,
        type: item.type,
        active: active,
        neutral: Math.abs(item.gain) < 1.0e-12
    };
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
    var changed = false;
    var bypass = Number(values[6]) !== 0;
    if (this.state.eqBypass !== bypass) {
        this.state.eqBypass = bypass;
        changed = true;
    }
    if (isFinite(selectedBankId) && selectedBankId >= 1 &&
        selectedBankId !== this.state.selectedBankId) {
        this.state.selectedBankId = selectedBankId;
        this.state.filterLimits = {};
        changed = true;
    }
    var operationAvailability = this.ReadOperationAvailability(values, selectedBankId);
    if (JSON.stringify(this.state.operationAvailability) !== JSON.stringify(operationAvailability) ||
        !this.state.hasEqSnapshot) {
        this.state.operationAvailability = operationAvailability;
        this.state.hasEqSnapshot = true;
        changed = true;
    }
    if (changed) this.RequestRedraw();
};

AnalyzerViewController.prototype.ReadOperationAvailability = function(values, selectedBankId) {
    var availability = {
        bypass: false,
        reset: false,
        join: false,
        commit: false,
        match: this.state.fitCurve.length > 1,
        clear: this.state.fitCurve.length > 0
    };
    if (values.length < 10 || Number(values[8]) !== 7) return availability;

    var selectedOccupied = false;
    var anyOccupied = false;
    var position = 9;
    for (var bankIndex = 0; bankIndex < 7; ++bankIndex) {
        if (position + 2 >= values.length) return availability;
        var bankId = Number(values[position++]);
        position++;
        var filterCount = Number(values[position++]);
        if (!isFinite(bankId) || !isFinite(filterCount) || filterCount < 0) return availability;
        var bankOccupied = false;
        for (var filterIndex = 0; filterIndex < filterCount; ++filterIndex) {
            if (position + 2 >= values.length) return availability;
            var filterId = Number(values[position++]);
            var filterBypass = Number(values[position++]) !== 0;
            var valueCount = Number(values[position++]);
            var definition = FilterDefinitionCatalog.Eq()[filterId];
            if (!isFinite(valueCount) || valueCount < 0 || position + valueCount > values.length) {
                return availability;
            }
            if (!filterBypass && definition) {
                for (var valueIndex = 0; valueIndex < valueCount &&
                    valueIndex < definition.parameters.length; ++valueIndex) {
                    var value = Number(values[position + valueIndex]);
                    var defaultValue = Number(definition.parameters[valueIndex].defaultValue);
                    if (isFinite(value) && isFinite(defaultValue) &&
                        Math.abs(value - defaultValue) > 1.0e-12) {
                        bankOccupied = true;
                        break;
                    }
                }
            }
            position += valueCount;
        }
        if (bankOccupied) {
            anyOccupied = true;
            if (bankId === selectedBankId) selectedOccupied = true;
        }
    }
    if (position !== values.length) return availability;
    availability.bypass = true;
    availability.reset = selectedOccupied;
    availability.join = selectedOccupied;
    availability.commit = anyOccupied;
    availability.match = this.state.fitCurve.length > 1;
    availability.clear = this.state.fitCurve.length > 0;
    return availability;
};

AnalyzerViewController.prototype.SetFilterLimits = function(values) {
    if (values.length !== 5) return;
    var bankId = Number(values[0]);
    var filterId = Number(values[1]);
    var parameterIndex = Number(values[2]);
    var minimum = Number(values[3]);
    var maximum = Number(values[4]);
    if (bankId !== this.state.selectedBankId || !isFinite(filterId) ||
        !isFinite(parameterIndex) || !isFinite(minimum) || !isFinite(maximum) ||
        maximum < minimum) return;
    this.state.filterLimits[String(filterId) + ":" + String(parameterIndex)] = {
        minimum: minimum,
        maximum: maximum
    };
};

AnalyzerViewController.prototype.SetEqPreview = function(values) {
    if (values.length !== 4) return;
    var bankId = Number(values[0]);
    var filterId = Number(values[1]);
    var parameterIndex = Number(values[2]);
    var value = Number(values[3]);
    if (bankId !== this.state.selectedBankId || !isFinite(value)) return;
    var definition = FilterDefinitionCatalog.Eq()[filterId];
    var handle = this.FindHandle(filterId);
    if (!definition || !handle || parameterIndex < 0 ||
        parameterIndex >= definition.parameters.length) return;
    var parameter = definition.parameters[parameterIndex].name;
    if (parameter === "gain") handle.gain = value;
    else if (parameter === "q") handle.q = value;
    else if (parameter === "freq" || parameter === "pivot") handle.frequency = value;
    else return;
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ParameterLimit = function(handle, parameterName, minimum, maximum) {
    var definition = FilterDefinitionCatalog.Eq()[handle.filterId];
    if (!definition) return { minimum: minimum, maximum: maximum };
    for (var index = 0; index < definition.parameters.length; ++index) {
        if (definition.parameters[index].name !== parameterName) continue;
        return this.state.filterLimits[
            String(handle.filterId) + ":" + String(index)
        ] || { minimum: minimum, maximum: maximum };
    }
    return { minimum: minimum, maximum: maximum };
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

AnalyzerViewController.prototype.OnClick = function(x, y, button, cmd, shift, capslock, option, ctrl) {
    if (this.HandleViewControl(x, y)) return;
    if (this.state.mode !== "spectrum") return;
    var handle = this.FindHandleAt(x, y);
    this.state.selectedHandleId = handle ? handle.filterId : null;
    if (handle && Boolean(ctrl)) {
        this.lastClickFilterId = null;
        this.ToggleBypass(handle);
        return;
    }
    if (handle && !handle.active) return;
    if (handle) {
        var now = new Date().getTime();
        if (this.lastClickFilterId === handle.filterId && now - this.lastClickTime <= 600) {
            this.lastClickFilterId = null;
            this.lastClickTime = 0;
            this.state.dragHandleId = null;
            this.state.dragStart = null;
            this.ResetFilter(handle);
            this.RequestRedraw();
            return;
        }
        this.lastClickFilterId = handle.filterId;
        this.lastClickTime = now;
    } else {
        this.lastClickFilterId = null;
    }
    this.state.dragHandleId = handle ? handle.filterId : null;
    if (!handle) return;
    var frequencyParameter = handle.type === "tilt" ? "pivot" : "freq";
    var qLimit = this.ParameterLimit(
        handle, "q", handle.qMinimum, handle.qMaximum);
    var frequencyLimit = this.ParameterLimit(
        handle, frequencyParameter,
        handle.frequencyMinimum, handle.frequencyMaximum);
    var gainLimit = this.ParameterLimit(
        handle, "gain", handle.gainMinimum, handle.gainMaximum);
    this.state.dragStart = {
        x: x,
        y: y,
        frequency: handle.frequency,
        gain: handle.gain,
        q: handle.q,
        editQ: Boolean(option),
        qMinimum: qLimit.minimum,
        qMaximum: qLimit.maximum,
        frequencyMinimum: frequencyLimit.minimum,
        frequencyMaximum: frequencyLimit.maximum,
        gainMinimum: gainLimit.minimum,
        gainMaximum: gainLimit.maximum
    };
    this.RequestRedraw();
};

AnalyzerViewController.prototype.OnDoubleClick = function(x, y, button, cmd, shift, capslock, option, ctrl) {
    if (this.HandleViewControl(x, y)) return;
    if (this.state.mode !== "spectrum") return;
    var handle = this.FindHandleAt(x, y);
    if (!handle) return;
    this.state.selectedHandleId = handle.filterId;
    this.state.dragHandleId = null;
    this.state.dragStart = null;
    this.ResetFilter(handle);
    this.RequestRedraw();
};

AnalyzerViewController.prototype.HandleViewControl = function(x, y) {
    if (y > analyzerControlsOptions.controlHeight) return false;
    var modeIndex = AnalyzerModeGroupIndexAt(x, y, mgraphics.size[0]);
    if (modeIndex === 0) {
        outlet(0, "view_mode", "spectrum");
        this.SetMode("spectrum");
        return true;
    }
    if (modeIndex === 1) {
        outlet(0, "view_mode", "analysis");
        this.SetMode("analysis");
        return true;
    }
    var scaleDb = ResolveAnalyzerScaleDb(this.state);
    var utilityIndex = AnalyzerUtilityGroupIndexAt(x, y, mgraphics.size[0], scaleDb);
    if (utilityIndex > 0 && !this.IsOperationAvailable(utilityIndex)) return true;
    if (utilityIndex === 0) {
        this.ToggleScale();
        return true;
    }
    if (utilityIndex === 1) {
        this.ToggleEqBypass();
        return true;
    }
    if (utilityIndex === 2) {
        this.ResetEq();
        return true;
    }
    if (utilityIndex === 3) {
        outlet(0, "bank.action", "join");
        return true;
    }
    if (utilityIndex === 4) {
        outlet(0, "bank.action", "commit");
        return true;
    }
    if (utilityIndex === 5) {
        this.StartFit();
        return true;
    }
    if (utilityIndex === 6) {
        this.ClearFitCurve();
        this.SendAnalyzerCommand("analyzer.clear", []);
        return true;
    }
    return false;
};

AnalyzerViewController.prototype.IsOperationAvailable = function(utilityIndex) {
    var availability = this.state.operationAvailability;
    if (utilityIndex === 1) return availability.bypass;
    if (utilityIndex === 2) return availability.reset;
    if (utilityIndex === 3) return availability.join;
    if (utilityIndex === 4) return availability.commit;
    if (utilityIndex === 5) return availability.match;
    if (utilityIndex === 6) return availability.clear;
    return false;
};

AnalyzerViewController.prototype.SendAnalyzerCommand = function(name, fields) {
    this.state.requestId += 1;
    outlet(0, ["command", 1, "analyzer", this.state.requestId, name]
        .concat(fields || []));
};

AnalyzerViewController.prototype.StartFit = function() {
    if (this.state.fitCurve.length < 2) return;
    this.state.requestId += 1;
    outlet(0, ["command", 1, "analyzer", this.state.requestId, "fit.start",
        this.state.fitCurve.length].concat(this.state.fitCurve));
};

AnalyzerViewController.prototype.ToggleEqBypass = function() {
    this.state.eqBypass = !this.state.eqBypass;
    outlet(0, "bank.action", "bypass", this.state.eqBypass ? 1 : 0);
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ResetEq = function() {
    outlet(0, "bank.action", "reset");
};

AnalyzerViewController.prototype.OnDrag = function(x, y, button, cmd, shift, capslock, option) {
    if (!button || this.state.dragHandleId === null || !this.state.dragStart) return;
    var handle = this.FindHandle(this.state.dragHandleId);
    if (!handle) return;
    var size = mgraphics.size;
    var bottom = spectrumGeometry.PlotBottom(size[1]);
    var start = this.state.dragStart;
    if (Boolean(option) && start.qMaximum > start.qMinimum) {
        var qRatio = spectrumGeometry.Clamp((start.y - y) / bottom, -1, 1);
        handle.q = spectrumGeometry.Clamp(start.q * Math.pow(8, qRatio), start.qMinimum, start.qMaximum);
        this.SendParameter(handle.filterId, "q", handle.q);
    }
    else {
        handle.frequency = spectrumGeometry.Clamp(
            spectrumGeometry.PointToFrequency(x, size[0]),
            start.frequencyMinimum,
            start.frequencyMaximum
        );
        handle.gain = spectrumGeometry.Clamp(
            start.gain + (start.y - y) / Math.max(1, bottom - spectrumGeometry.PlotTop()) * spectrumOptions.scaleDb * 2,
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
    var bottom = spectrumGeometry.PlotBottom(size[1]);
    var radius = spectrumOptions.handleHitRadius;
    var closest = null;
    var closestDistance = radius * radius;
    for (var index = 0; index < this.state.handles.length; ++index) {
        var handle = this.state.handles[index];
        var handleX = handle.type === "gain"
            ? spectrumOptions.gainHandleX
            : spectrumGeometry.FrequencyToX(handle.frequency, size[0]);
        var dx = x - handleX;
        var dy = y - spectrumGeometry.DbToY(handle.gain, bottom);
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

AnalyzerViewController.prototype.ToggleBypass = function(handle) {
    handle.active = !handle.active;
    var curve = this.state.filterCurves[String(handle.filterId)];
    if (curve) curve.active = handle.active;
    this.state.requestId += 1;
    outlet(0, "command", 1, "spectrum", this.state.requestId, "eq.set_bypass",
        this.state.selectedBankId, handle.filterId, handle.active ? 0 : 1);
    this.state.dragHandleId = null;
    this.state.dragStart = null;
    this.RequestRedraw();
};

AnalyzerViewController.prototype.ResetFilter = function(handle) {
    var now = new Date().getTime();
    if (now - this.lastResetTime < 250) return;
    this.lastResetTime = now;
    this.state.requestId += 1;
    outlet(0, [
        "command", 1, "spectrum", this.state.requestId, "eq.reset_filter",
        this.state.selectedBankId, handle.filterId
    ]);
};
