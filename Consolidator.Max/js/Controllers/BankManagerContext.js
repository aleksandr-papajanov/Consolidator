class BankManagerContext
{
    constructor(viewModel, state, uiTarget, instanceId, protocol, transactions,
        onSnapshotAccepted, scope)
    {
        this.viewModel = viewModel;
        this.state = state;
        this.uiTarget = uiTarget;
        this.instanceId = instanceId;
        this.protocol = protocol;
        this.transactions = transactions;
        this.scope = scope || { mode: "local" };
        this.onSnapshotAccepted = onSnapshotAccepted || (() => {});
    }
}

module.exports = {
    BankManagerContext: BankManagerContext
};
