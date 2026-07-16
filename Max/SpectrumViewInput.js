function list() {
    var index = inlet;

    if (index === 5) {
        return;
    }

    if (index < 0 || index >= curves.length) {
        return;
    }

    var incoming = arrayfromargs(arguments).map(Number);
    curves[index] = incoming;

    mgraphics.redraw();
}

function handle() {
    if (inlet !== 5) {
        return;
    }

    var args = arrayfromargs(arguments);
    if (args.length < 5) {
        return;
    }

    var item = {
        slot: Number(args[0]),
        frequency: Number(args[1]),
        gain: Number(args[2]),
        type: String(args[3]),
        active: Number(args[4]) !== 0,
        q: Number(args[5] || 0),
        qMin: Number(args[6] || 0),
        qMax: Number(args[7] || 0)
    };
    var existing = findHandle(item.slot);
    if (existing >= 0) {
        handles[existing] = item;
    }
    else {
        handles.push(item);
    }

    if (sameHandle(draggedHandle, item)) {
        draggedHandle = item;
    }

    mgraphics.redraw();
}

function filter_curve() {
    if (inlet !== 5) {
        return;
    }

    var args = arrayfromargs(arguments);
    if (args.length < 13) {
        return;
    }

    var filterId = Number(args[0]);
    var active = Number(args[1]) !== 0;
    var item = {
        color: {
            r: clamp(Number(args[2]), 0, 1),
            g: clamp(Number(args[3]), 0, 1),
            b: clamp(Number(args[4]), 0, 1),
            a: clamp(Number(args[5]), 0, 1)
        },
        curve: args.slice(12).map(Number)
    };
    var marker = {
        slot: filterId,
        frequency: Number(args[6]),
        gain: Number(args[7]),
        type: String(args[8]),
        active: active,
        q: Number(args[9] || 0),
        qMin: Number(args[10] || 0),
        qMax: Number(args[11] || 0)
    };
    var existingHandle = findHandle(filterId);
    if (existingHandle >= 0) {
        handles[existingHandle] = marker;
    }
    else {
        handles.push(marker);
    }
    if (sameHandle(draggedHandle, marker)) {
        draggedHandle = marker;
    }

    if (active && item.curve.length > 1) {
        filterCurves[filterId] = item;
    }
    else {
        delete filterCurves[filterId];
        var handleIndex = findHandle(filterId);
        if (handleIndex >= 0) {
            handles[handleIndex].active = false;
        }
    }

    mgraphics.redraw();
}

function onclick(x, y, button, cmd, shift, capslock, option) {
    var point = toLocalPoint(x, y);
    var nearbyCandidates = findHandlesAt(point.x, point.y);
    var selectedIndex = findCandidateIndex(nearbyCandidates, selectedHandleSlot);
    var sameSelection = selectedIndex >= 0 &&
        Math.abs(point.x - selectionX) <= visualSettings.handleCycleDistance &&
        Math.abs(point.y - selectionY) <= visualSettings.handleCycleDistance;

    var candidates;
    if (sameSelection && selectionCandidates.length > 0) {
        candidates = selectionCandidates;
        selectedIndex = findCandidateIndex(candidates, selectedHandleSlot);
    }
    else {
        candidates = nearbyCandidates;
        selectionCandidates = candidates;
        selectionX = point.x;
        selectionY = point.y;
        selectedIndex = findCandidateIndex(candidates, selectedHandleSlot);
    }

    selectionIndex = sameSelection ? selectedIndex : (candidates.length > 0 ? 0 : -1);

    draggedHandle = selectionIndex >= 0 ? candidates[selectionIndex] : null;
    draggedHandleSlot = draggedHandle ? draggedHandle.slot : null;
    selectedHandleSlot = draggedHandleSlot;
    clickWasRepeat = sameSelection;
    clickMoved = false;
    if (draggedHandle) {
        draggingWithAlt = isAltModifierDown(option) && draggedHandle.qMax > draggedHandle.qMin;
        setDragAnchor(point, draggingWithAlt);
    }
}

function ondrag(x, y, button, cmd, shift, capslock, option) {
    if (button === 0) {
        finishClick();
        return;
    }

    if (draggedHandleSlot === null) {
        return;
    }

    draggedHandle = getHandleBySlot(draggedHandleSlot);
    if (!draggedHandle || !draggedHandle.active) {
        return;
    }

    clickMoved = true;

    var point = toLocalPoint(x, y);
    var size = mgraphics.size;
    var w = size[0];
    var plotBottom = getPlotBottom(size[1]);
    var nextAltMode = isAltModifierDown(option, arguments[6]) &&
        draggedHandle.qMax > draggedHandle.qMin;
    if (nextAltMode !== draggingWithAlt) {
        draggingWithAlt = nextAltMode;
        setDragAnchor(point, draggingWithAlt);
    }

    if (draggingWithAlt) {
        var qNormalized = clamp(
            dragStartQNormalized - (point.y - dragStartY) / plotBottom * qSensitivity,
            0,
            1
        );
        var q = denormalizeQ(qNormalized, draggedHandle.qMin, draggedHandle.qMax);
        draggedHandle.q = q;
        outlet(0, "edit", draggedHandle.slot, "q", qNormalized);
        return;
    }

    var frequency = clamp(
        dragStartFrequency * Math.pow(displayMaxFrequency / displayMinFrequency, (point.x - dragStartX) / w),
        displayMinFrequency,
        displayMaxFrequency
    );
    var gain = clamp(
        dragStartGain - (point.y - dragStartY) / plotBottom * (maxDb - minDb),
        minDb,
        maxDb
    );
    draggedHandle.frequency = frequency;
    draggedHandle.gain = gain;
    outlet(0, "edit", draggedHandle.slot, frequency, gain);
}

function setDragAnchor(point, qMode) {
    dragStartX = point.x;
    dragStartY = point.y;
    dragStartFrequency = draggedHandle.frequency;
    dragStartGain = draggedHandle.gain;
    dragStartQNormalized = qMode
        ? normalizeQ(draggedHandle.q, draggedHandle.qMin, draggedHandle.qMax)
        : 0;
}

function normalizeQ(value, minValue, maxValue) {
    if (minValue <= 0 || maxValue <= minValue) {
        return 0;
    }
    return clamp(
        Math.log(clamp(value, minValue, maxValue) / minValue) /
            Math.log(maxValue / minValue),
        0,
        1
    );
}

function denormalizeQ(value, minValue, maxValue) {
    if (minValue <= 0 || maxValue <= minValue) {
        return minValue;
    }
    return minValue * Math.pow(maxValue / minValue, clamp(value, 0, 1));
}

function isAltModifierDown(option, argumentOption) {
    return Boolean(option) || Boolean(argumentOption);
}

function onmouseup() {
    finishClick();
}

function finishClick() {
    if (clickWasRepeat && !clickMoved && selectionCandidates.length > 1) {
        selectionIndex = (selectionIndex + 1) % selectionCandidates.length;
        selectedHandleSlot = selectionCandidates[selectionIndex].slot;
    }

    draggedHandle = null;
    draggedHandleSlot = null;
    clickWasRepeat = false;
    clickMoved = false;
    draggingWithAlt = false;
    mgraphics.redraw();
}

function sameHandle(left, right) {
    if (!left || !right) {
        return left === right;
    }
    return left.slot === right.slot;
}

function findHandlesAt(x, y) {
    var size = mgraphics.size;
    var candidates = [];

    for (var i = 0; i < handles.length; i++) {
        if (!handles[i].active) {
            continue;
        }

        var handleX = frequencyToX(handles[i].frequency, size[0]);
        var handleY = dbToY(handles[i].gain, getPlotBottom(size[1]));
        var distance = Math.sqrt(
            Math.pow(x - handleX, 2) + Math.pow(y - handleY, 2)
        );
        if (distance <= visualSettings.handleHitRadius) {
            candidates.push({ handle: handles[i], distance: distance, zIndex: i });
        }
    }

    candidates.sort(function (left, right) {
        var leftSelected = left.handle.slot === selectedHandleSlot;
        var rightSelected = right.handle.slot === selectedHandleSlot;
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

function findCandidateIndex(candidates, slot) {
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

function smooth(value) {
    smoothing = clamp(Number(value), 0, 0.98);
}

function q_sensitivity(value) {
    // The inlet value is normalized; around 0.63 matches the default response.
    qSensitivity = 0.25 + clamp(Number(value), 0, 1) * 1.75;
}

function clear() {
    for (var i = 0; i < curves.length; i++) {
        curves[i] = [];
    }

    filterCurves = {};
    handles = [];
    draggedHandle = null;
    draggedHandleSlot = null;
    selectedHandleSlot = null;
    clickWasRepeat = false;
    clickMoved = false;
    selectionCandidates = [];
    selectionIndex = -1;

    mgraphics.redraw();
}

function target_size(size) {
    return;
}

function range(minValue, maxValue) {
    minDb = Number(minValue);
    maxDb = Number(maxValue);
    dbRangeIndex = -1;
    mgraphics.redraw();
}

function range_mode(value) {
    setDbRangeMode(Number(value));
}

function toggle_range() {
    setDbRangeMode(dbRangeIndex === 0 ? 1 : 0);
}

function setDbRangeMode(index) {
    if (index < 0 || index >= dbRangePresets.length) {
        return;
    }

    dbRangeIndex = index;
    minDb = dbRangePresets[index].min;
    maxDb = dbRangePresets[index].max;
    mgraphics.redraw();
}

function findHandle(slot) {
    for (var i = 0; i < handles.length; i++) {
        if (handles[i].slot === slot) {
            return i;
        }
    }

    return -1;
}

function getHandleBySlot(slot) {
    var index = findHandle(slot);
    return index >= 0 ? handles[index] : null;
}

function findHandleAt(x, y) {
    var point = toLocalPoint(x, y);
    var size = mgraphics.size;
    var w = size[0];
    var plotBottom = getPlotBottom(size[1]);
    var closest = null;
    var closestDistance = 12;

    for (var i = 0; i < handles.length; i++) {
        if (!handles[i].active) {
            continue;
        }

        var handleX = frequencyToX(handles[i].frequency, w);
        var handleY = dbToY(handles[i].gain, plotBottom);
        var distance = Math.sqrt(Math.pow(point.x - handleX, 2) + Math.pow(point.y - handleY, 2));
        if (distance < closestDistance) {
            closest = handles[i];
            closestDistance = distance;
        }
    }

    return closest;
}

function toLocalPoint(x, y) {
    var rect = box.rect;
    var size = mgraphics.size;
    if (x > size[0] && x >= rect[0]) {
        x -= rect[0];
    }
    if (y > size[1] && y >= rect[1]) {
        y -= rect[1];
    }

    return {
        x: clamp(x, 0, size[0]),
        y: clamp(y, 0, size[1])
    };
}
// Max messages, selection state, and pointer interaction.
