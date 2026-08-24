function clampPresentationValue(value, minimum, maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

function normalizePresentationValue(value, minimum, maximum, logarithmic) {
    if (maximum === minimum) {
        return 0;
    }

    if (logarithmic && minimum > 0 && maximum > 0 && value > 0) {
        return clampPresentationValue(
            Math.log(value / minimum) / Math.log(maximum / minimum),
            0,
            1
        );
    }

    return clampPresentationValue(
        (value - minimum) / (maximum - minimum),
        0,
        1
    );
}

function denormalizePresentationValue(value, minimum, maximum, logarithmic) {
    var normalized = clampPresentationValue(value, 0, 1);
    if (logarithmic && minimum > 0 && maximum > 0) {
        return minimum * Math.pow(maximum / minimum, normalized);
    }
    return minimum + (maximum - minimum) * normalized;
}
