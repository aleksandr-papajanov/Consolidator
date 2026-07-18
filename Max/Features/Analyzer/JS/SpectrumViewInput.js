include("../../Shared/JS/DictionaryReader.js");
include("../../Shared/JS/Messages/MessageEnvelope.js");

SpectrumViewController.prototype.list = function() {
    var index = inlet;

    if (index === 3) {
        return;
    }

    if (index < 0 || index >= spectrumState.curves.length) {
        return;
    }

    spectrumState.curves[index] = arrayfromargs(arguments);

    mgraphics.redraw();
}

SpectrumViewController.prototype.handle = function() {
    if (inlet !== 3) {
        return;
    }

    var values = arrayfromargs(arguments);
    if (values.length !== 8) {
        return;
    }

    var item = {
        slot: Number(values[0]),
        frequency: Number(values[1]),
        gain: Number(values[2]),
        type: String(values[3]),
        active: Number(values[4]) !== 0,
        q: Number(values[5]),
        qMin: Number(values[6]),
        qMax: Number(values[7])
    };
    var existing = this.findHandle(item.slot);
    if (existing >= 0) {
        spectrumState.handles[existing] = item;
    }
    else {
        spectrumState.handles.push(item);
    }

    if (this.sameHandle(spectrumState.draggedHandle, item)) {
        spectrumState.draggedHandle = item;
    }

    mgraphics.redraw();
}

SpectrumViewController.prototype.filter_curve = function() {
    if (inlet !== 3) {
        return;
    }

    var values = arrayfromargs(arguments);
    if (values.length < 13) {
        return;
    }

    var filterId = Number(values[0]);
    var active = Number(values[1]) !== 0;
    var item = {
        color: {
            r: Number(values[2]),
            g: Number(values[3]),
            b: Number(values[4]),
            a: Number(values[5])
        },
        curve: values.slice(12).map(Number)
    };
    var marker = {
        slot: filterId,
        frequency: Number(values[6]),
        gain: Number(values[7]),
        type: String(values[8]),
        active: active,
        q: Number(values[9]),
        qMin: Number(values[10]),
        qMax: Number(values[11])
    };
    var existingHandle = this.findHandle(filterId);
    if (existingHandle >= 0) {
        spectrumState.handles[existingHandle] = marker;
    }
    else {
        spectrumState.handles.push(marker);
    }
    if (this.sameHandle(spectrumState.draggedHandle, marker)) {
        spectrumState.draggedHandle = marker;
    }

    if (active && item.curve.length > 1) {
        spectrumState.filterCurves[filterId] = item;
    }
    else {
        delete spectrumState.filterCurves[filterId];
        var handleIndex = this.findHandle(filterId);
        if (handleIndex >= 0) {
            spectrumState.handles[handleIndex].active = false;
        }
    }

    mgraphics.redraw();
}

SpectrumViewController.prototype.onclick = function(x, y, button, cmd, shift, capslock, option) {
    var point = this.toLocalPoint(x, y);
    var nearbyCandidates = this.findHandlesAt(point.x, point.y);
    var selectedIndex = this.findCandidateIndex(nearbyCandidates, spectrumState.selectedHandleSlot);
    var sameSelection = selectedIndex >= 0 &&
        Math.abs(point.x - spectrumState.selectionX) <= spectrumState.visualSettings.handleCycleDistance &&
        Math.abs(point.y - spectrumState.selectionY) <= spectrumState.visualSettings.handleCycleDistance;

    var candidates;
    if (sameSelection && spectrumState.selectionCandidates.length > 0) {
        candidates = spectrumState.selectionCandidates;
        selectedIndex = this.findCandidateIndex(candidates, spectrumState.selectedHandleSlot);
    }
    else {
        candidates = nearbyCandidates;
        spectrumState.selectionCandidates = candidates;
        spectrumState.selectionX = point.x;
        spectrumState.selectionY = point.y;
        selectedIndex = this.findCandidateIndex(candidates, spectrumState.selectedHandleSlot);
    }

    spectrumState.selectionIndex = sameSelection ? selectedIndex : (candidates.length > 0 ? 0 : -1);

    spectrumState.draggedHandle = spectrumState.selectionIndex >= 0 ? candidates[spectrumState.selectionIndex] : null;
    spectrumState.draggedHandleSlot = spectrumState.draggedHandle ? spectrumState.draggedHandle.slot : null;
    spectrumState.selectedHandleSlot = spectrumState.draggedHandleSlot;
    spectrumState.clickWasRepeat = sameSelection;
    spectrumState.clickMoved = false;
    if (spectrumState.draggedHandle) {
        spectrumState.draggingWithAlt = this.isAltModifierDown(option) && spectrumState.draggedHandle.qMax > spectrumState.draggedHandle.qMin;
        this.setDragAnchor(point, spectrumState.draggingWithAlt);
    }
}

SpectrumViewController.prototype.ondrag = function(x, y, button, cmd, shift, capslock, option) {
    if (button === 0) {
        this.finishClick();
        return;
    }

    if (spectrumState.draggedHandleSlot === null) {
        return;
    }

    spectrumState.draggedHandle = this.getHandleBySlot(spectrumState.draggedHandleSlot);
    if (!spectrumState.draggedHandle || !spectrumState.draggedHandle.active) {
        return;
    }

    spectrumState.clickMoved = true;

    var point = this.toLocalPoint(x, y);
    var size = mgraphics.size;
    var w = size[0];
    var plotBottom = this.getPlotBottom(size[1]);
    var nextAltMode = this.isAltModifierDown(option, arguments[6]) &&
        spectrumState.draggedHandle.qMax > spectrumState.draggedHandle.qMin;
    if (nextAltMode !== spectrumState.draggingWithAlt) {
        spectrumState.draggingWithAlt = nextAltMode;
        this.setDragAnchor(point, spectrumState.draggingWithAlt);
    }

    if (spectrumState.draggingWithAlt) {
        var qNormalized = this.clamp(
            spectrumState.dragStartQNormalized - (point.y - spectrumState.dragStartY) / plotBottom * spectrumState.qSensitivity,
            0,
            1
        );
        var q = this.denormalizeQ(qNormalized, spectrumState.draggedHandle.qMin, spectrumState.draggedHandle.qMax);
        spectrumState.draggedHandle.q = q;
        this.sendEditMessage(MessageEnvelope.create("filter.edit", "filter", {
            filterId: spectrumState.draggedHandle.slot,
            parameter: "q",
            value: qNormalized
        }, "spectrum"));
        return;
    }

    var frequency = this.clamp(
        spectrumState.dragStartFrequency * Math.pow(spectrumState.displayMaxFrequency / spectrumState.displayMinFrequency, (point.x - spectrumState.dragStartX) / w),
        spectrumState.displayMinFrequency,
        spectrumState.displayMaxFrequency
    );
    var gain = this.clamp(
        spectrumState.dragStartGain - (point.y - spectrumState.dragStartY) / plotBottom * (spectrumState.maxDb - spectrumState.minDb),
        spectrumState.minDb,
        spectrumState.maxDb
    );
    spectrumState.draggedHandle.frequency = frequency;
    spectrumState.draggedHandle.gain = gain;
    this.sendEditMessage(MessageEnvelope.create("filter.edit", "filter", {
        filterId: spectrumState.draggedHandle.slot,
        frequency: frequency,
        gain: gain
    }, "spectrum"));
}

SpectrumViewController.prototype.sendEditMessage = function(message) {
    if (!message) {
        return;
    }
    var dictionary = message.toMaxDictionary();
    outlet(0, "message", dictionary.name);
}

SpectrumViewController.prototype.setDragAnchor = function(point, qMode) {
    spectrumState.dragStartX = point.x;
    spectrumState.dragStartY = point.y;
    spectrumState.dragStartFrequency = spectrumState.draggedHandle.frequency;
    spectrumState.dragStartGain = spectrumState.draggedHandle.gain;
    spectrumState.dragStartQNormalized = qMode
        ? this.normalizeQ(spectrumState.draggedHandle.q, spectrumState.draggedHandle.qMin, spectrumState.draggedHandle.qMax)
        : 0;
}

SpectrumViewController.prototype.normalizeQ = function(value, minValue, maxValue) {
    if (minValue <= 0 || maxValue <= minValue) {
        return 0;
    }
    return this.clamp(
        Math.log(this.clamp(value, minValue, maxValue) / minValue) /
            Math.log(maxValue / minValue),
        0,
        1
    );
}

SpectrumViewController.prototype.denormalizeQ = function(value, minValue, maxValue) {
    if (minValue <= 0 || maxValue <= minValue) {
        return minValue;
    }
    return minValue * Math.pow(maxValue / minValue, this.clamp(value, 0, 1));
}

SpectrumViewController.prototype.isAltModifierDown = function(option, argumentOption) {
    return Boolean(option) || Boolean(argumentOption);
}

SpectrumViewController.prototype.onmouseup = function() {
    this.finishClick();
}

SpectrumViewController.prototype.finishClick = function() {
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

SpectrumViewController.prototype.sameHandle = function(left, right) {
    if (!left || !right) {
        return left === right;
    }
    return left.slot === right.slot;
}

SpectrumViewController.prototype.findHandlesAt = function(x, y) {
    var size = mgraphics.size;
    var candidates = [];

    for (var i = 0; i < spectrumState.handles.length; i++) {
        if (!spectrumState.handles[i].active) {
            continue;
        }

        var handleX = this.frequencyToX(spectrumState.handles[i].frequency, size[0]);
        var handleY = this.dbToY(spectrumState.handles[i].gain, this.getPlotBottom(size[1]));
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

SpectrumViewController.prototype.findCandidateIndex = function(candidates, slot) {
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

SpectrumViewController.prototype.smooth = function(value) {
    spectrumState.smoothing = this.clamp(Number(value), 0, 0.98);
}

SpectrumViewController.prototype.q_sensitivity = function(value) {
    // The inlet value is normalized; around 0.63 matches the default response.
    spectrumState.qSensitivity = 0.25 + this.clamp(Number(value), 0, 1) * 1.75;
}

SpectrumViewController.prototype.clear = function() {
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

SpectrumViewController.prototype.clear_difference = function() {
    spectrumState.curves[2] = [];
    mgraphics.redraw();
}

SpectrumViewController.prototype.target_size = function(size) {};

SpectrumViewController.prototype.range = function(minValue, maxValue) {
    spectrumState.minDb = Number(minValue);
    spectrumState.maxDb = Number(maxValue);
    spectrumState.dbRangeIndex = -1;
    mgraphics.redraw();
}

SpectrumViewController.prototype.range_mode = function(value) {
    this.setDbRangeMode(Number(value));
}

SpectrumViewController.prototype.toggle_range = function() {
    this.setDbRangeMode(spectrumState.dbRangeIndex === 0 ? 1 : 0);
}

SpectrumViewController.prototype.setDbRangeMode = function(index) {
    if (index < 0 || index >= spectrumState.dbRangePresets.length) {
        return;
    }

    spectrumState.dbRangeIndex = index;
    spectrumState.minDb = spectrumState.dbRangePresets[index].min;
    spectrumState.maxDb = spectrumState.dbRangePresets[index].max;
    mgraphics.redraw();
}

SpectrumViewController.prototype.findHandle = function(slot) {
    for (var i = 0; i < spectrumState.handles.length; i++) {
        if (spectrumState.handles[i].slot === slot) {
            return i;
        }
    }

    return -1;
}

SpectrumViewController.prototype.getHandleBySlot = function(slot) {
    var index = this.findHandle(slot);
    return index >= 0 ? spectrumState.handles[index] : null;
}

SpectrumViewController.prototype.findHandleAt = function(x, y) {
    var point = this.toLocalPoint(x, y);
    var size = mgraphics.size;
    var w = size[0];
    var plotBottom = this.getPlotBottom(size[1]);
    var closest = null;
    var closestDistance = 12;

    for (var i = 0; i < spectrumState.handles.length; i++) {
        if (!spectrumState.handles[i].active) {
            continue;
        }

        var handleX = this.frequencyToX(spectrumState.handles[i].frequency, w);
        var handleY = this.dbToY(spectrumState.handles[i].gain, plotBottom);
        var distance = Math.sqrt(Math.pow(point.x - handleX, 2) + Math.pow(point.y - handleY, 2));
        if (distance < closestDistance) {
            closest = spectrumState.handles[i];
            closestDistance = distance;
        }
    }

    return closest;
}

SpectrumViewController.prototype.toLocalPoint = function(x, y) {
    var rect = box.rect;
    var size = mgraphics.size;
    if (x > size[0] && x >= rect[0]) {
        x -= rect[0];
    }
    if (y > size[1] && y >= rect[1]) {
        y -= rect[1];
    }

    return {
        x: this.clamp(x, 0, size[0]),
        y: this.clamp(y, 0, size[1])
    };
}
// Max messages, selection state, and pointer interaction.
