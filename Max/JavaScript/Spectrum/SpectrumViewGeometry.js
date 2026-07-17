SpectrumViewController.prototype.formatDbLabel = function(db) {
    if (db === 0) {
        return "0";
    }

    return (db > 0 ? "+" : "") + String(db);
}

SpectrumViewController.prototype.isMajorFrequency = function(freq) {
    for (var i = 0; i < spectrumState.majorFrequencies.length; i++) {
        if (spectrumState.majorFrequencies[i] === freq) {
            return true;
        }
    }

    return false;
}

SpectrumViewController.prototype.getDbGridStep = function() {
    return spectrumState.dbRangeIndex === 0 ? 6 : 12;
}

SpectrumViewController.prototype.frequencyToX = function(freq, w) {
    freq = this.clamp(freq, spectrumState.displayMinFrequency, spectrumState.displayMaxFrequency);

    var x = 0;

    for (var i = 0; i < spectrumState.frequencyBands.length; i++) {
        var band = spectrumState.frequencyBands[i];
        var bandWidth = w * band.width;

        if (freq <= band.max || i === spectrumState.frequencyBands.length - 1) {
            var t = this.normalizedLog(freq, band.min, band.max);
            return x + t * bandWidth;
        }

        x += bandWidth;
    }

    return w;
}

SpectrumViewController.prototype.xToFrequency = function(x, w) {
    x = this.clamp(x, 0, w);
    var offset = 0;

    for (var i = 0; i < spectrumState.frequencyBands.length; i++) {
        var band = spectrumState.frequencyBands[i];
        var bandWidth = w * band.width;
        if (x <= offset + bandWidth || i === spectrumState.frequencyBands.length - 1) {
            var t = this.clamp((x - offset) / bandWidth, 0, 1);
            return band.min * Math.pow(band.max / band.min, t);
        }

        offset += bandWidth;
    }

    return spectrumState.displayMaxFrequency;
}

SpectrumViewController.prototype.binToX = function(index, total, w) {
    if (total <= 1) {
        return 0;
    }

    var t = index / (total - 1);
    var freq = spectrumState.curveMinFrequency * Math.pow(spectrumState.displayMaxFrequency / spectrumState.curveMinFrequency, t);
    return this.frequencyToX(freq, w);
}

SpectrumViewController.prototype.normalizedLog = function(value, minValue, maxValue) {
    value = this.clamp(value, minValue, maxValue);
    var logMin = Math.log(minValue);
    var logMax = Math.log(maxValue);
    return (Math.log(value) - logMin) / (logMax - logMin);
}

SpectrumViewController.prototype.formatFrequencyLabel = function(freq) {
    if (freq >= 1000) {
        return String(freq / 1000) + "k";
    }

    return String(freq);
}

SpectrumViewController.prototype.estimateLabelWidth = function(label) {
    return label.length * 4.6;
}

SpectrumViewController.prototype.getPlotBottom = function(h) {
    return h - 18;
}

SpectrumViewController.prototype.dbToY = function(db, plotBottom) {
    db = this.clamp(db, spectrumState.minDb, spectrumState.maxDb);
    var norm = (db - spectrumState.minDb) / (spectrumState.maxDb - spectrumState.minDb);
    return plotBottom - norm * plotBottom;
}

SpectrumViewController.prototype.yToDb = function(y, plotBottom) {
    var norm = 1 - this.clamp(y / plotBottom, 0, 1);
    return spectrumState.minDb + norm * (spectrumState.maxDb - spectrumState.minDb);
}

SpectrumViewController.prototype.clamp = function(v, min, max) {
    return Math.max(min, Math.min(max, v));
}
// Frequency and screen-coordinate conversion helpers.
