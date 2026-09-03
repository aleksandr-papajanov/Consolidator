class UiTarget
{
    constructor(targetState)
    {
        this.targetState = targetState;
    }
    
    show(instanceId, bankId, snapshotContext, callback)
    {
        return this.targetState.selectTarget(
            instanceId, bankId, snapshotContext, callback);
    }
    
    destroy()
    {
        this.targetState = null;
    }
}

module.exports = {
    UiTarget: UiTarget
};
