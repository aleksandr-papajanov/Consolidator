const EqualizerFilterDefinitions = [
    { type: "gain", fixedQ: 0.707, parameters: {
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "tilt", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 1000 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "low_shelf", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 100 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "high_shelf", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 10000 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "bell", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 1000 },
        q: { minimum: 0.1, maximum: 10, defaultValue: 0.707 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "bell", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 2000 },
        q: { minimum: 0.1, maximum: 10, defaultValue: 0.707 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "bell", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 4000 },
        q: { minimum: 0.1, maximum: 10, defaultValue: 0.707 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } }
];

const DetectorFilterDefinitions = [
    { type: "low_shelf", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 100 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } },
    { type: "bell", fixedQ: 0.707, parameters: {
        frequency: { minimum: 20, maximum: 20000, defaultValue: 1000 },
        q: { minimum: 0.1, maximum: 10, defaultValue: 0.707 },
        gain: { minimum: -24, maximum: 24, defaultValue: 0 }
    } }
];

module.exports = {
    EqualizerFilterDefinitions: EqualizerFilterDefinitions,
    DetectorFilterDefinitions: DetectorFilterDefinitions
};
