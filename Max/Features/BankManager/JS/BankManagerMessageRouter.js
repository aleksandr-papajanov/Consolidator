function BankManagerMessageRouter(manager) {
    this.manager = manager;
}

BankManagerMessageRouter.prototype.HandleLocal = function(name, values) {
    if (name === "processor_match_operation") {
        this.manager.HandleProcessorMatchOperation(values);
    } else if (name === "processor_bypass_operation") {
        this.manager.HandleProcessorBypassOperation(values);
    } else if (name === "processor_detector_reset") {
        this.manager.HandleProcessorDetectorReset(values);
    } else if (name === "eq_filter_reset") {
        this.manager.ExecuteFilterReset(Number(values[0]), Number(values[1]));
    } else if (name === "bank.action") {
        this.manager.ExecuteOperation(String(values[0]), values[1]);
    }
};

BankManagerMessageRouter.prototype.Handle = function(inletIndex, name, values) {
    if (inletIndex === 1) {
        this.manager.HandleGlobal(name, values);
        return;
    }
    if (name === "coordinator_directory") {
        this.manager.ParseCoordinatorDirectory(values);
        return;
    }
    this.HandleLocal(name, values);
};
