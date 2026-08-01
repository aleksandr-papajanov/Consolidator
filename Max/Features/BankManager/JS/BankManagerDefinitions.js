function BankManagerDefinitions() {
    this.filterParameters = {};
    this.filterTypes = {};
    this.filterDefaultBypass = {};
    this.processorRanges = {};
    this.processorDefaults = {};
    this.Load();
}

BankManagerDefinitions.prototype.Load = function() {
    var eqDefinitions = FilterDefinitionCatalog.Eq();
    for (var filterId in eqDefinitions) {
        if (!eqDefinitions.hasOwnProperty(filterId)) continue;
        var filter = eqDefinitions[filterId];
        this.filterParameters[filterId] = filter.parameters;
        this.filterTypes[filterId] = filter.type;
        this.filterDefaultBypass[filterId] = Boolean(filter.defaultBypass);
    }
    var processorDefinitions = FilterDefinitionCatalog.Processors();
    for (var device in processorDefinitions) {
        if (!processorDefinitions.hasOwnProperty(device)) continue;
        this.processorRanges[device] = {};
        this.processorDefaults[device] = {};
        var parameters = processorDefinitions[device].parameters;
        for (var index = 0; index < parameters.length; ++index) {
            var parameter = parameters[index];
            this.processorRanges[device][parameter.name] = {
                minimum: parameter.minimum,
                maximum: parameter.maximum,
                logarithmic: parameter.logarithmic
            };
            this.processorDefaults[device][parameter.name] = parameter.defaultValue;
        }
    }
};
