class AnalyzerCurvePublisher
{
    constructor()
    {
        this.listeners = [];
        this.previousCurves = null;
        this.previousCombined = null;
    }

    subscribe(callback, immediate, curves, combinedCurve)
    {
        this.listeners.push(callback);
        if (immediate && ((curves || []).length > 0 || combinedCurve))
        {
            callback(curves || [], combinedCurve);
        }
        return () => {
            this.listeners = this.listeners.filter((listener) => listener !== callback);
        };
    }

    publish(curves, combinedCurve)
    {
        const changedCurves = (curves || []).filter((curve) => {
            if (!this.previousCurves)
            {
                return true;
            }
            const previous = this.previousCurves.find((candidate) => {
                return candidate.id === curve.id;
            });
            return !sameCurve(previous, curve);
        });
        const combinedChanged = !sameCurve(this.previousCombined, combinedCurve);
        this.previousCurves = curves;
        this.previousCombined = combinedCurve;
        this.listeners.slice().forEach((listener) => {
            listener(changedCurves, combinedChanged ? combinedCurve : null);
        });
    }

    clear()
    {
        this.listeners = [];
        this.previousCurves = null;
        this.previousCombined = null;
    }
}

function sameCurve(first, second)
{
    if (!first || !second || first.active !== second.active ||
            first.values.length !== second.values.length)
    {
        return false;
    }
    for (let index = 0; index < first.values.length; index += 1)
    {
        if (first.values[index] !== second.values[index])
        {
            return false;
        }
    }
    return true;
}

module.exports = {
    AnalyzerCurvePublisher: AnalyzerCurvePublisher
};
