const { presentationBindingValue } = require(
    "../../../Shared/Presenters/PresentationBinding.js");
const { BiquadCalculator } = require("./BiquadCalculator.js");

const CURVE_POINT_COUNT = 256;
const DEFAULT_SAMPLE_RATE = 48000;

class AnalyzerCurveCalculator
{
    constructor(parameters, bankBypass, scale, sampleRate)
    {
        this.parameters = parameters || [];
        this.bankBypass = bankBypass || null;
        this.scale = scale;
        this.sampleRate = Number(sampleRate) > 0
            ? Number(sampleRate)
            : DEFAULT_SAMPLE_RATE;
        this.frequencies = this.createFrequencies();
    }

    calculate(previews)
    {
        const bankActive = !Boolean(presentationBindingValue(this.bankBypass, false));
        const responses = this.parameters.map((parameter, index) => {
            return this.calculateFilter(
                index + 1,
                parameter,
                Boolean(presentationBindingValue(parameter.enabled, true)),
                previews[index + 1]
            );
        });
        const combinedDecibels = new Array(CURVE_POINT_COUNT).fill(0);
        if (bankActive)
        {
            responses.forEach((response) => {
                if (!response.active)
                {
                    return;
                }
                response.decibels.forEach((value, index) => {
                    combinedDecibels[index] += value;
                });
            });
        }

        return {
            curves: responses.map((response) => ({
                id: response.id,
                active: response.active,
                values: response.values
            })),
            combinedCurve: {
                active: bankActive,
                values: combinedDecibels.map(normalizeDecibels)
            }
        };
    }

    calculateFilter(id, parameter, enabled, preview)
    {
        const definition = parameter.definition || {
            type: parameter.type || "bell",
            fixedQ: 1,
            parameters: {}
        };
        const frequency = preview && preview.frequency !== undefined
            ? Number(preview.frequency)
            : Number(presentationBindingValue(parameter.frequency, 1000));
        const q = preview && preview.q !== undefined
            ? Number(preview.q)
            : parameter.q
                ? Number(presentationBindingValue(parameter.q, definition.fixedQ || 1))
                : Number(definition.fixedQ || 1);
        const gain = preview && preview.gain !== undefined
            ? Number(preview.gain)
            : Number(presentationBindingValue(parameter.gain, 0));
        const valid = enabled && isFinite(gain) &&
            (definition.type === "gain" ||
                isFinite(frequency) && isFinite(q) && frequency > 0 && q > 0);
        const coefficients = valid
            ? BiquadCalculator.calculate(
                definition.type,
                frequency,
                q,
                gain,
                this.sampleRate
            )
            : null;
        const decibels = this.frequencies.map((pointFrequency) => {
            return coefficients
                ? BiquadCalculator.decibelsAt(coefficients, pointFrequency, this.sampleRate)
                : 0;
        });
        return {
            id: id,
            active: enabled,
            decibels: decibels,
            values: decibels.map(normalizeDecibels)
        };
    }

    createFrequencies()
    {
        const frequencies = [];
        for (let point = 0; point < CURVE_POINT_COUNT; point += 1)
        {
            const normalized = point / (CURVE_POINT_COUNT - 1);
            frequencies.push(this.scale.frequencyMinimum * Math.pow(
                this.scale.frequencyMaximum / this.scale.frequencyMinimum,
                normalized
            ));
        }
        return frequencies;
    }
}

function normalizeDecibels(decibels)
{
    return Math.max(0, Math.min(1, 1 - (decibels + 24) / 48));
}

module.exports = {
    AnalyzerCurveCalculator: AnalyzerCurveCalculator
};
