function UiTarget(targetState) {
    this.targetState = targetState;
}

UiTarget.prototype.show = function (instanceId, bankId) {
    return this.targetState.selectTarget(instanceId, bankId);
};

UiTarget.prototype.destroy = function () {
    this.targetState = null;
};
