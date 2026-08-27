class BiquadCalculator
{
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
        return 20 * Math.log10(Math.max(
            BiquadCalculator.magnitudeAt(coefficients, frequencyHz, sampleRate),
            1e-12));
    }
}

module.exports = {
    BiquadCalculator: BiquadCalculator
};
