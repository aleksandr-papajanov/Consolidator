class BankManagerContext
{
    constructor(viewModel, state, uiTarget, instanceId, protocol, transactions,
        onSnapshotAccepted)
    {
        this.viewModel = viewModel;
        this.state = state;
        this.uiTarget = uiTarget;
        this.instanceId = instanceId;
        this.protocol = protocol;
        this.transactions = transactions;
        this.onSnapshotAccepted = onSnapshotAccepted || (() => {});
    }
}

module.exports = {
    BankManagerContext: BankManagerContext
};
