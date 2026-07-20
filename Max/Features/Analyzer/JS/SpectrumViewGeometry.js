SpectrumViewController.prototype.FormatDbLabel = function(db) {
    if (db === 0) {
        return "0";
    }

    return (db > 0 ? "+" : "") + String(db);
}

SpectrumViewController.prototype.IsMajorFrequency = function(freq) {
    for (var i = 0; i < spectrumState.majorFrequencies.length; i++) {
        if (spectrumState.majorFrequencies[i] === freq) {
            return true;
        }
    }

    return false;
}

SpectrumViewController.prototype.GetDbGridStep = function() {
    return spectrumState.dbRangeIndex === 0 ? 6 : 12;
}

SpectrumViewController.prototype.FrequencyToX = function(freq, w) {
    freq = this.Clamp(freq, spectrumState.displayMinFrequency, spectrumState.displayMaxFrequency);

    var x = 0;

    for (var i = 0; i < spectrumState.frequencyBands.length; i++) {
        var band = spectrumState.frequencyBands[i];
        var bandWidth = w * band.width;

        if (freq <= band.max || i === spectrumState.frequencyBands.length - 1) {
            var t = this.NormalizedLog(freq, band.min, band.max);
            return x + t * bandWidth;
        }

        x += bandWidth;
    }

    return w;
}

SpectrumViewController.prototype.BinToX = function(index, total, w) {
    if (total <= 1) {
        return 0;
    }

    var t = index / (total - 1);
    var freq = spectrumState.curveMinFrequency * Math.pow(
        spectrumState.curveMaxFrequency / spectrumState.curveMinFrequency, t);
    return this.FrequencyToX(freq, w);
}

SpectrumViewController.prototype.NormalizedLog = function(value, minValue, maxValue) {
    value = this.Clamp(value, minValue, maxValue);
    var logMin = Math.log(minValue);
    var logMax = Math.log(maxValue);
    return (Math.log(value) - logMin) / (logMax - logMin);
}

SpectrumViewController.prototype.FormatFrequencyLabel = function(freq) {
    if (freq >= 1000) {
        return String(freq / 1000) + "k";
    }

    return String(freq);
}

SpectrumViewController.prototype.EstimateLabelWidth = function(label) {
    return label.length * 4.6;
}

SpectrumViewController.prototype.GetPlotBottom = function(h) {
    return h - 18;
}

SpectrumViewController.prototype.DbToY = function(db, plotBottom) {
    db = this.Clamp(db, spectrumState.minDb, spectrumState.maxDb);
    var norm = (db - spectrumState.minDb) / (spectrumState.maxDb - spectrumState.minDb);
    return plotBottom - norm * plotBottom;
}

SpectrumViewController.prototype.Clamp = function(v, min, max) {
    return Math.max(min, Math.min(max, v));
}
// Frequency and screen-coordinate conversion helpers.
