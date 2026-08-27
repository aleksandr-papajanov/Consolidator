class UiTarget
{
    constructor(targetState)
    {
        this.targetState = targetState;
    }
    
    show(instanceId, bankId, callback)
    {
        return this.targetState.selectTarget(instanceId, bankId, callback);
    }
    
    destroy()
    {
        this.targetState = null;
    }
}

module.exports = {
    UiTarget: UiTarget
};
