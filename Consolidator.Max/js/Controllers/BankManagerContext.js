class BankManagerContext
{
    constructor(viewModel, state, uiTarget, instanceId, protocol, transactions)
    {
        this.viewModel = viewModel;
        this.state = state;
        this.uiTarget = uiTarget;
        this.instanceId = instanceId;
        this.protocol = protocol;
        this.transactions = transactions;
    }
}

module.exports = {
    BankManagerContext: BankManagerContext
};
