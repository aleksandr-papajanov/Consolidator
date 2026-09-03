class BiquadCalculator
{
    static calculate(type, frequencyHz, q, gainDb, sampleRate)
    {
        if (type === "gain") {
            return { constantDb: gainDb };
        }
        if (gainDb === 0) {
            return { b0: 1, b1: 0, b2: 0, a1: 0, a2: 0 };
        }
        if (type === "low_shelf" || type === "high_shelf") {
            return BiquadCalculator.calculateShelf(
                type, frequencyHz, q, gainDb, sampleRate);
        }
        if (type === "tilt") {
            return {
                low: BiquadCalculator.calculateShelf(
                    "low_shelf", frequencyHz, q, -gainDb, sampleRate),
                high: BiquadCalculator.calculateShelf(
                    "high_shelf", frequencyHz, q, gainDb, sampleRate)
            };
        }
        return BiquadCalculator.calculateBell(frequencyHz, q, gainDb, sampleRate);
    }

    static calculateShelf(type, frequencyHz, q, gainDb, sampleRate)
    {
        let omega = 2 * Math.PI * frequencyHz / sampleRate;
        let sine = Math.sin(omega);
        let cosine = Math.cos(omega);
        let amplitude = Math.pow(10, gainDb / 40);
        let alpha = sine / (2 * q);
        let beta = 2 * Math.sqrt(amplitude) * alpha;
        let lowShelf = type === "low_shelf";
        let b0 = lowShelf
            ? amplitude * ((amplitude + 1) - (amplitude - 1) * cosine + beta)
            : amplitude * ((amplitude + 1) + (amplitude - 1) * cosine + beta);
        let b1 = lowShelf
            ? 2 * amplitude * ((amplitude - 1) - (amplitude + 1) * cosine)
            : -2 * amplitude * ((amplitude - 1) + (amplitude + 1) * cosine);
        let b2 = lowShelf
            ? amplitude * ((amplitude + 1) - (amplitude - 1) * cosine - beta)
            : amplitude * ((amplitude + 1) + (amplitude - 1) * cosine - beta);
        let a0 = lowShelf
            ? (amplitude + 1) + (amplitude - 1) * cosine + beta
            : (amplitude + 1) - (amplitude - 1) * cosine + beta;
        let a1 = lowShelf
            ? -2 * ((amplitude - 1) + (amplitude + 1) * cosine)
            : 2 * ((amplitude - 1) - (amplitude + 1) * cosine);
        let a2 = lowShelf
            ? (amplitude + 1) + (amplitude - 1) * cosine - beta
            : (amplitude + 1) - (amplitude - 1) * cosine - beta;
        return { b0: b0 / a0, b1: b1 / a0, b2: b2 / a0,
            a1: a1 / a0, a2: a2 / a0 };
    }
    static calculateBell(frequencyHz, q, gainDb, sampleRate)
    {
        let omega = 2 * Math.PI * frequencyHz / sampleRate;
        let sine = Math.sin(omega);
        let cosine = Math.cos(omega);
        let alpha = sine / (2 * q);
        let amplitude = Math.pow(10, gainDb / 40);
        let inverseA0 = 1 / (1 + alpha / amplitude);

        return {
            b0: (1 + alpha * amplitude) * inverseA0,
            b1: -2 * cosine * inverseA0,
            b2: (1 - alpha * amplitude) * inverseA0,
            a1: -2 * cosine * inverseA0,
            a2: (1 - alpha / amplitude) * inverseA0
        };
    }

    static magnitudeAt(coefficients, frequencyHz, sampleRate)
    {
        let omega = 2 * Math.PI * frequencyHz / sampleRate;
        let cosine = Math.cos(omega);
        let sine = Math.sin(omega);
        let cosine2 = Math.cos(2 * omega);
        let sine2 = Math.sin(2 * omega);
        let numeratorReal = coefficients.b0 + coefficients.b1 * cosine +
            coefficients.b2 * cosine2;
        let numeratorImaginary = -coefficients.b1 * sine -
            coefficients.b2 * sine2;
        let denominatorReal = 1 + coefficients.a1 * cosine +
            coefficients.a2 * cosine2;
        let denominatorImaginary = -coefficients.a1 * sine -
            coefficients.a2 * sine2;
        let numeratorMagnitude = Math.sqrt(
            numeratorReal * numeratorReal + numeratorImaginary * numeratorImaginary);
        let denominatorMagnitude = Math.sqrt(
            denominatorReal * denominatorReal + denominatorImaginary * denominatorImaginary);

        return numeratorMagnitude / Math.max(denominatorMagnitude, 1e-12);
    }

    static decibelsAt(coefficients, frequencyHz, sampleRate)
    {
        if (coefficients.constantDb !== undefined) {
            return coefficients.constantDb;
        }
        if (coefficients.low && coefficients.high) {
            return BiquadCalculator.decibelsAt(
                coefficients.low, frequencyHz, sampleRate) +
                BiquadCalculator.decibelsAt(
                    coefficients.high, frequencyHz, sampleRate);
        }
        return 20 * Math.log10(Math.max(
            BiquadCalculator.magnitudeAt(coefficients, frequencyHz, sampleRate),
            1e-12));
    }
}

module.exports = {
    BiquadCalculator: BiquadCalculator
};
