var FilterDefinitionCatalog = {
    Eq: function() {
        return {
            1: { id: 1, type: "tilt", defaultBypass: false, parameters: [
                { name: "gain", minimum: -15, maximum: 15, logarithmic: false, defaultValue: 0 },
                { name: "pivot", minimum: 40, maximum: 18000, logarithmic: true, defaultValue: 1000 }
            ] },
            2: { id: 2, type: "lowshelf", defaultBypass: false, parameters: [
                { name: "gain", minimum: -15, maximum: 15, logarithmic: false, defaultValue: 0 },
                { name: "freq", minimum: 40, maximum: 800, logarithmic: true, defaultValue: 200 }
            ] },
            3: { id: 3, type: "highshelf", defaultBypass: false, parameters: [
                { name: "gain", minimum: -15, maximum: 15, logarithmic: false, defaultValue: 0 },
                { name: "freq", minimum: 1000, maximum: 18000, logarithmic: true, defaultValue: 8000 }
            ] },
            4: FilterDefinitionCatalog.Peak(4, 100, 8000, 1000, 0.2, 1, 1),
            5: FilterDefinitionCatalog.Peak(5, 40, 18000, 3000, 0.2, 1, 1),
            6: FilterDefinitionCatalog.Peak(6, 40, 18000, 500, 1, 7, 3),
            9: { id: 9, type: "gain", defaultBypass: false, parameters: [
                { name: "gain", minimum: -15, maximum: 15, logarithmic: false, defaultValue: 0 }
            ] }
        };
    },

    Detector: function() {
        return {
            1: {
                id: 1, type: "peak", defaultBypass: false, parameters: [
                    { name: "gain", minimum: -24, maximum: 24, logarithmic: false, defaultValue: 0 },
                    { name: "frequency", minimum: 20, maximum: 20000, logarithmic: true, defaultValue: 200 },
                    { name: "q", minimum: 0.2, maximum: 8, logarithmic: true, defaultValue: 0.707 }
                ]
            },
            2: {
                id: 2, type: "peak", defaultBypass: false, parameters: [
                    { name: "gain", minimum: -24, maximum: 24, logarithmic: false, defaultValue: 0 },
                    { name: "frequency", minimum: 20, maximum: 20000, logarithmic: true, defaultValue: 4000 },
                    { name: "q", minimum: 0.2, maximum: 8, logarithmic: true, defaultValue: 0.707 }
                ]
            }
        };
    },

    Processors: function() {
        return {
            input_gain: { id: "input_gain", parameters: [
                FilterDefinitionCatalog.Parameter("gain", -38, 38, false, 0)
            ] },
            compressor: { id: "compressor", parameters: [
                FilterDefinitionCatalog.Parameter("attack", 0.1, 500, true, 10),
                FilterDefinitionCatalog.Parameter("release", 5, 2000, true, 100),
                FilterDefinitionCatalog.Parameter("threshold", -60, 0, false, -18),
                FilterDefinitionCatalog.Parameter("output", -24, 24, false, 0),
                FilterDefinitionCatalog.Parameter("mix", 0, 1, false, 1)
            ].concat(FilterDefinitionCatalog.DetectorParameters()) },
            saturator: { id: "saturator", parameters: [
                FilterDefinitionCatalog.Parameter("saturation", 0, 1, false, 0),
                FilterDefinitionCatalog.Parameter("output", -24, 24, false, 0)
            ].concat(FilterDefinitionCatalog.DetectorParameters()) },
            output_gain: { id: "output_gain", parameters: [
                FilterDefinitionCatalog.Parameter("gain", -38, 38, false, 0)
            ] }
        };
    },

    Peak: function(
        id,
        frequencyMinimum,
        frequencyMaximum,
        frequencyDefault,
        qMinimum,
        qMaximum,
        qDefault
    ) {
        return { id: id, type: "peak", defaultBypass: false, parameters: [
            { name: "gain", minimum: -15, maximum: 15, logarithmic: false, defaultValue: 0 },
            { name: "freq", minimum: frequencyMinimum, maximum: frequencyMaximum, logarithmic: true, defaultValue: frequencyDefault },
            { name: "q", minimum: qMinimum, maximum: qMaximum, logarithmic: true, defaultValue: qDefault }
        ] };
    },

    DetectorFilter: function(id, frequencyDefault) {
        return { id: id, type: "peak", defaultBypass: false, parameters: [
            { name: "gain", minimum: -24, maximum: 24, logarithmic: false, defaultValue: 0 },
            { name: "frequency", minimum: 20, maximum: 20000, logarithmic: true, defaultValue: frequencyDefault },
            { name: "q", minimum: 0.2, maximum: 8, logarithmic: true, defaultValue: 0.707 }
        ] };
    },

    DetectorParameters: function() {
        var result = [];
        var detectorDefinitions = FilterDefinitionCatalog.Detector();
        for (var id in detectorDefinitions) {
            if (!detectorDefinitions.hasOwnProperty(id)) continue;
            var definition = detectorDefinitions[id];
            for (var index = 0; index < definition.parameters.length; index++) {
                var parameter = definition.parameters[index];
                result.push(FilterDefinitionCatalog.Parameter(
                    "detector." + id + "." + parameter.name,
                    parameter.minimum, parameter.maximum,
                    parameter.logarithmic, parameter.defaultValue));
            }
            result.push(FilterDefinitionCatalog.Parameter(
                "detector." + id + ".bypass", 0, 1, false, 0));
        }
        return result;
    },

    Parameter: function(name, minimum, maximum, logarithmic, defaultValue) {
        return {
            name: name,
            minimum: minimum,
            maximum: maximum,
            logarithmic: logarithmic,
            defaultValue: defaultValue
        };
    }
};
