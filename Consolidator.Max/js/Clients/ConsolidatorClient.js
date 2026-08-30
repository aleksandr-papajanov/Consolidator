const { NativeProtocolClient } = require("./NativeProtocolClient.js");
const { StateClient } = require("./StateClient.js");
const { TransactionClient } = require("./TransactionClient.js");
const { TargetStateClient } = require("./TargetStateClient.js");
const { UiTarget } = require("./UiTarget.js");
const { RegistryClient } = require("./RegistryClient.js");
const { UiEditScope } = require("../ViewModels/UiEditScope.js");

class ConsolidatorClient
{
    constructor(source, send)
    {
        this.protocol = new NativeProtocolClient(source, send);
        this.scope = new UiEditScope();
        this.state = new StateClient(this.protocol, this.scope);
        this.transactions = new TransactionClient(this.protocol);
        this.targetState = new TargetStateClient(this.protocol, this.state);
        this.uiTarget = new UiTarget(this.targetState);
        this.registry = new RegistryClient(this.protocol);
    }
    
    initialize(callback)
    {
        return this.protocol.initialize(callback);
    }
    
    setInstanceActive(active, callback)
    {
        return this.protocol.request(
            "set_instance_active",
            [active ? 1 : 0],
            callback);
    }
    
    handleControl(selector, args)
    {
        this.protocol.handleControl(selector, args);
    }
    
    destroy()
    {
        this.state.destroy();
        this.transactions.destroy();
        this.targetState.destroy();
        this.uiTarget.destroy();
        this.registry.destroy();
        this.protocol.destroy();
    }
}

module.exports = {
    ConsolidatorClient: ConsolidatorClient
};
