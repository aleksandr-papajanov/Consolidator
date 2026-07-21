SpectrumViewController.prototype.List = function() {
    var index = inlet;

    if (index === 3) {
        return;
    }

    if (index < 0 || index >= spectrumState.curves.length) {
        return;
    }

    var values = arrayfromargs(arguments);
    if (spectrumState.curvePointCount > 0 && values.length !== spectrumState.curvePointCount) return;
    spectrumState.curves[index] = values;

    mgraphics.redraw();
}

SpectrumViewController.prototype.FilterCurve = function() {
    if (inlet !== 3) {
        return;
    }

    var values = arrayfromargs(arguments);
    if (values.length < 9) {
        return;
    }

    var filterId = Number(values[0]);
    var active = Number(values[1]) !== 0;
    var item = {
        color: spectrumState.filterColors[String(filterId)] || spectrumState.visualSettings.handleFallbackColor,
        curve: values.slice(8).map(Number)
    };
    if (spectrumState.curvePointCount > 0 && item.curve.length !== spectrumState.curvePointCount) return;
    var marker = {
        slot: filterId,
        frequency: Number(values[2]),
        gain: Number(values[3]),
        type: String(values[4]),
        active: active,
        q: Number(values[5]),
        qMin: Number(values[6]),
        qMax: Number(values[7])
    };
    var existingHandle = this.FindHandle(filterId);
    if (existingHandle >= 0) {
        spectrumState.handles[existingHandle] = marker;
    }
    else {
        spectrumState.handles.push(marker);
    }
    if (this.SameHandle(spectrumState.draggedHandle, marker)) {
        spectrumState.draggedHandle = marker;
    }

    if (active && item.curve.length > 1) {
        spectrumState.filterCurves[filterId] = item;
    }
    else {
        delete spectrumState.filterCurves[filterId];
        var handleIndex = this.FindHandle(filterId);
        if (handleIndex >= 0) {
            spectrumState.handles[handleIndex].active = false;
        }
    }

    mgraphics.redraw();
}

SpectrumViewController.prototype.CurveSettings = function(minimumHz, maximumHz, pointCount) {
    var minimum = Number(minimumHz);
    var maximum = Number(maximumHz);
    var count = Number(pointCount);
    if (!isFinite(minimum) || !isFinite(maximum) || !isFinite(count) ||
        minimum <= 0 || maximum <= minimum || count < 2 || Math.floor(count) !== count) return;
    spectrumState.curveMinFrequency = minimum;
    spectrumState.curveMaxFrequency = maximum;
    spectrumState.curvePointCount = count;
    mgraphics.redraw();
}

SpectrumViewController.prototype.OnClick = function(x, y, button, cmd, shift, capslock, option) {
    var point = this.ToLocalPoint(x, y);
    var nearbyCandidates = this.FindHandlesAt(point.x, point.y);
    var selectedIndex = this.FindCandidateIndex(nearbyCandidates, spectrumState.selectedHandleSlot);
    var sameSelection = selectedIndex >= 0 &&
        Math.abs(point.x - spectrumState.selectionX) <= spectrumState.visualSettings.handleCycleDistance &&
        Math.abs(point.y - spectrumState.selectionY) <= spectrumState.visualSettings.handleCycleDistance;

    var candidates;
    if (sameSelection && spectrumState.selectionCandidates.length > 0) {
        candidates = spectrumState.selectionCandidates;
        selectedIndex = this.FindCandidateIndex(candidates, spectrumState.selectedHandleSlot);
    }
    else {
        candidates = nearbyCandidates;
        spectrumState.selectionCandidates = candidates;
        spectrumState.selectionX = point.x;
        spectrumState.selectionY = point.y;
        selectedIndex = this.FindCandidateIndex(candidates, spectrumState.selectedHandleSlot);
    }

    spectrumState.selectionIndex = sameSelection ? selectedIndex : (candidates.length > 0 ? 0 : -1);

    spectrumState.draggedHandle = spectrumState.selectionIndex >= 0 ? candidates[spectrumState.selectionIndex] : null;
    spectrumState.draggedHandleSlot = spectrumState.draggedHandle ? spectrumState.draggedHandle.slot : null;
    spectrumState.selectedHandleSlot = spectrumState.draggedHandleSlot;
    spectrumState.clickWasRepeat = sameSelection;
    spectrumState.clickMoved = false;
    if (spectrumState.draggedHandle) {
        spectrumState.draggingWithAlt = this.IsAltModifierDown(option) && spectrumState.draggedHandle.qMax > spectrumState.draggedHandle.qMin;
        this.SetDragAnchor(point, spectrumState.draggingWithAlt);
    }
}

SpectrumViewController.prototype.OnDrag = function(x, y, button, cmd, shift, capslock, option) {
    if (button === 0) {
        this.FinishClick();
        return;
    }

    if (spectrumState.draggedHandleSlot === null) {
        return;
    }

    spectrumState.draggedHandle = this.GetHandleBySlot(spectrumState.draggedHandleSlot);
    if (!spectrumState.draggedHandle || !spectrumState.draggedHandle.active) {
        return;
    }

    spectrumState.clickMoved = true;

    var point = this.ToLocalPoint(x, y);
    var size = mgraphics.size;
    var w = this.GetSpectrumPlotWidth(size[0]);
    var plotBottom = this.GetPlotBottom(size[1]);
    var nextAltMode = this.IsAltModifierDown(option, arguments[6]) &&
        spectrumState.draggedHandle.qMax > spectrumState.draggedHandle.qMin;
    if (nextAltMode !== spectrumState.draggingWithAlt) {
        spectrumState.draggingWithAlt = nextAltMode;
        this.SetDragAnchor(point, spectrumState.draggingWithAlt);
    }

    if (spectrumState.draggingWithAlt) {
        var qNormalized = this.Clamp(
            spectrumState.dragStartQNormalized - (point.y - spectrumState.dragStartY) / plotBottom * spectrumState.qSensitivity,
            0,
            1
        );
        var q = this.DenormalizeQ(qNormalized, spectrumState.draggedHandle.qMin, spectrumState.draggedHandle.qMax);
        this.SendEditCommand(spectrumState.draggedHandle.slot, "q", q);
        return;
    }

    var frequency = this.Clamp(
        spectrumState.dragStartFrequency * Math.pow(spectrumState.displayMaxFrequency / spectrumState.displayMinFrequency, (point.x - spectrumState.dragStartX) / w),
        spectrumState.displayMinFrequency,
        spectrumState.displayMaxFrequency
    );
    var gain = this.Clamp(
        spectrumState.dragStartGain - (point.y - spectrumState.dragStartY) / plotBottom * (spectrumState.maxDb - spectrumState.minDb),
        spectrumState.minDb,
        spectrumState.maxDb
    );
    var parameter = spectrumState.draggedHandle.type === "tilt" ? "pivot" : "freq";
    this.SendEditCommand(spectrumState.draggedHandle.slot, parameter, frequency);
    this.SendEditCommand(spectrumState.draggedHandle.slot, "gain", gain);
}

SpectrumViewController.prototype.SendEditCommand = function(filterId, parameter, value) {
    spectrumState.requestId += 1;
    outlet(0, "command", 1, "spectrum", spectrumState.requestId, "eq.set_parameter",
        spectrumState.selectedBankId, filterId, parameter, value);
}

SpectrumViewController.prototype.HandleBusMessage = function(values) {
    if (!values || values.length < 7 || String(values[0]) !== "snapshot" ||
        Number(values[1]) !== 1 || String(values[2]) !== "host" || String(values[3]) !== "eq") return;
    var selectedBank = Number(values[5]);
    if (isFinite(selectedBank) && selectedBank >= 1) spectrumState.selectedBankId = selectedBank;
};

SpectrumViewController.prototype.SetDragAnchor = function(point, qMode) {
    spectrumState.dragStartX = point.x;
    spectrumState.dragStartY = point.y;
    spectrumState.dragStartFrequency = spectrumState.draggedHandle.frequency;
    spectrumState.dragStartGain = spectrumState.draggedHandle.gain;
    spectrumState.dragStartQNormalized = qMode
        ? this.NormalizeQ(spectrumState.draggedHandle.q, spectrumState.draggedHandle.qMin, spectrumState.draggedHandle.qMax)
        : 0;
}

SpectrumViewController.prototype.NormalizeQ = function(value, minValue, maxValue) {
    if (minValue <= 0 || maxValue <= minValue) {
        return 0;
    }
    return this.Clamp(
        Math.log(this.Clamp(value, minValue, maxValue) / minValue) /
            Math.log(maxValue / minValue),
        0,
        1
    );
}

SpectrumViewController.prototype.DenormalizeQ = function(value, minValue, maxValue) {
    if (minValue <= 0 || maxValue <= minValue) {
        return minValue;
    }
    return minValue * Math.pow(maxValue / minValue, this.Clamp(value, 0, 1));
}

SpectrumViewController.prototype.IsAltModifierDown = function(option, argumentOption) {
    return Boolean(option) || Boolean(argumentOption);
}

SpectrumViewController.prototype.OnMouseUp = function() {
    this.FinishClick();
}

SpectrumViewController.prototype.FinishClick = function() {
    if (spectrumState.clickWasRepeat && !spectrumState.clickMoved && spectrumState.selectionCandidates.length > 1) {
        spectrumState.selectionIndex = (spectrumState.selectionIndex + 1) % spectrumState.selectionCandidates.length;
        spectrumState.selectedHandleSlot = spectrumState.selectionCandidates[spectrumState.selectionIndex].slot;
    }

    spectrumState.draggedHandle = null;
    spectrumState.draggedHandleSlot = null;
    spectrumState.clickWasRepeat = false;
    spectrumState.clickMoved = false;
    spectrumState.draggingWithAlt = false;
    mgraphics.redraw();
}

SpectrumViewController.prototype.SameHandle = function(left, right) {
    if (!left || !right) {
        return left === right;
    }
    return left.slot === right.slot;
}

SpectrumViewController.prototype.FindHandlesAt = function(x, y) {
    var size = mgraphics.size;
    var plotWidth = this.GetSpectrumPlotWidth(size[0]);
    var candidates = [];

    for (var i = 0; i < spectrumState.handles.length; i++) {
        if (!spectrumState.handles[i].active) {
            continue;
        }

        var handleX = this.FrequencyToX(spectrumState.handles[i].frequency, plotWidth);
        var handleY = this.DbToY(spectrumState.handles[i].gain, this.GetPlotBottom(size[1]));
        var distance = Math.sqrt(
            Math.pow(x - handleX, 2) + Math.pow(y - handleY, 2)
        );
        if (distance <= spectrumState.visualSettings.handleHitRadius) {
            candidates.push({ handle: spectrumState.handles[i], distance: distance, zIndex: i });
        }
    }

    candidates.sort(function (left, right) {
        var leftSelected = left.handle.slot === spectrumState.selectedHandleSlot;
        var rightSelected = right.handle.slot === spectrumState.selectedHandleSlot;
        if (leftSelected !== rightSelected) {
            return leftSelected ? -1 : 1;
        }
        if (left.distance !== right.distance) {
            return right.zIndex - left.zIndex;
        }
        return right.zIndex - left.zIndex;
    });

    return candidates.map(function (item) {
        return item.handle;
    });
}

SpectrumViewController.prototype.FindCandidateIndex = function(candidates, slot) {
    if (slot === null || slot === undefined) {
        return -1;
    }
    for (var i = 0; i < candidates.length; i++) {
        if (candidates[i].slot === slot) {
            return i;
        }
    }
    return -1;
}

SpectrumViewController.prototype.Smooth = function(value) {
    spectrumState.smoothing = this.Clamp(Number(value), 0, 0.98);
}

SpectrumViewController.prototype.QSensitivity = function(value) {
    // The inlet value is normalized; around 0.63 matches the default response.
    spectrumState.qSensitivity = 0.25 + this.Clamp(Number(value), 0, 1) * 1.75;
}

SpectrumViewController.prototype.Clear = function() {
    for (var i = 0; i < spectrumState.curves.length; i++) {
        spectrumState.curves[i] = [];
    }

    spectrumState.filterCurves = {};
    spectrumState.handles = [];
    spectrumState.draggedHandle = null;
    spectrumState.draggedHandleSlot = null;
    spectrumState.selectedHandleSlot = null;
    spectrumState.clickWasRepeat = false;
    spectrumState.clickMoved = false;
    spectrumState.selectionCandidates = [];
    spectrumState.selectionIndex = -1;
    mgraphics.redraw();
}

SpectrumViewController.prototype.ClearDifference = function() {
    spectrumState.curves[2] = [];
    mgraphics.redraw();
}

SpectrumViewController.prototype.Range = function(minValue, maxValue) {
    spectrumState.minDb = Number(minValue);
    spectrumState.maxDb = Number(maxValue);
    spectrumState.dbRangeIndex = -1;
    mgraphics.redraw();
}

SpectrumViewController.prototype.RangeMode = function(value) {
    this.SetDbRangeMode(Number(value));
}

SpectrumViewController.prototype.ToggleRange = function() {
    this.SetDbRangeMode(spectrumState.dbRangeIndex === 0 ? 1 : 0);
}

SpectrumViewController.prototype.SetDbRangeMode = function(index) {
    if (index < 0 || index >= spectrumState.dbRangePresets.length) {
        return;
    }

    spectrumState.dbRangeIndex = index;
    spectrumState.minDb = spectrumState.dbRangePresets[index].min;
    spectrumState.maxDb = spectrumState.dbRangePresets[index].max;
    mgraphics.redraw();
}

SpectrumViewController.prototype.FindHandle = function(slot) {
    for (var i = 0; i < spectrumState.handles.length; i++) {
        if (spectrumState.handles[i].slot === slot) {
            return i;
        }
    }

    return -1;
}

SpectrumViewController.prototype.GetHandleBySlot = function(slot) {
    var index = this.FindHandle(slot);
    return index >= 0 ? spectrumState.handles[index] : null;
}

SpectrumViewController.prototype.ToLocalPoint = function(x, y) {
    var rect = box.rect;
    var size = mgraphics.size;
    if (x > size[0] && x >= rect[0]) {
        x -= rect[0];
    }
    if (y > size[1] && y >= rect[1]) {
        y -= rect[1];
    }

    return {
        x: this.Clamp(x, 0, size[0]),
        y: this.Clamp(y, 0, size[1])
    };
}
// Max messages, selection state, and pointer interaction.
