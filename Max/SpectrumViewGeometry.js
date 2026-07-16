function formatDbLabel(db) {
    if (db === 0) {
        return "0";
    }

    return (db > 0 ? "+" : "") + String(db);
}

function isMajorFrequency(freq) {
    for (var i = 0; i < majorFrequencies.length; i++) {
        if (majorFrequencies[i] === freq) {
            return true;
        }
    }

    return false;
}

function getDbGridStep() {
    return dbRangeIndex === 0 ? 6 : 12;
}

function frequencyToX(freq, w) {
    freq = clamp(freq, displayMinFrequency, displayMaxFrequency);

    var x = 0;

    for (var i = 0; i < frequencyBands.length; i++) {
        var band = frequencyBands[i];
        var bandWidth = w * band.width;

        if (freq <= band.max || i === frequencyBands.length - 1) {
            var t = normalizedLog(freq, band.min, band.max);
            return x + t * bandWidth;
        }

        x += bandWidth;
    }

    return w;
}

function xToFrequency(x, w) {
    x = clamp(x, 0, w);
    var offset = 0;

    for (var i = 0; i < frequencyBands.length; i++) {
        var band = frequencyBands[i];
        var bandWidth = w * band.width;
        if (x <= offset + bandWidth || i === frequencyBands.length - 1) {
            var t = clamp((x - offset) / bandWidth, 0, 1);
            return band.min * Math.pow(band.max / band.min, t);
        }

        offset += bandWidth;
    }

    return displayMaxFrequency;
}

function binToX(index, total, w) {
    if (total <= 1) {
        return 0;
    }

    var t = index / (total - 1);
    var freq = curveMinFrequency * Math.pow(displayMaxFrequency / curveMinFrequency, t);
    return frequencyToX(freq, w);
}

function normalizedLog(value, minValue, maxValue) {
    value = clamp(value, minValue, maxValue);
    var logMin = Math.log(minValue);
    var logMax = Math.log(maxValue);
    return (Math.log(value) - logMin) / (logMax - logMin);
}

function formatFrequencyLabel(freq) {
    if (freq >= 1000) {
        return String(freq / 1000) + "k";
    }

    return String(freq);
}

function estimateLabelWidth(label) {
    return label.length * 4.6;
}

function getPlotBottom(h) {
    return h - 18;
}

function dbToY(db, plotBottom) {
    db = clamp(db, minDb, maxDb);
    var norm = (db - minDb) / (maxDb - minDb);
    return plotBottom - norm * plotBottom;
}

function yToDb(y, plotBottom) {
    var norm = 1 - clamp(y / plotBottom, 0, 1);
    return minDb + norm * (maxDb - minDb);
}

function clamp(v, min, max) {
    return Math.max(min, Math.min(max, v));
}
// Frequency and screen-coordinate conversion helpers.
