const { StateResponseAssembler } = require("./StateResponseAssembler.js");
const { encodeStatePath, encodeStateValue } = require("./StateWireCodec.js");

const MAX_BATCH_SIZE = 16;

class StateClient
{
    // Managed resolves ordinary relative writes from the source selection.
    // Topology writes and targeted resets carry their target explicitly.
    constructor(protocol, scope)
    {
        this.protocol = protocol;
        this.scope = scope || { mode: "local" };
        this.responseAssembler = new StateResponseAssembler();
        this.unsubscribeProtocol = [
            protocol.on("state_begin", (args) => this.responseAssembler.begin(args)),
            protocol.on("state_entry", (args) => this.responseAssembler.addEntry(args)),
            protocol.on("state_done", (args) => this.handleResponseDone(args))
        ];
    }

    get responses()
    {
        return this.responseAssembler.responses;
    }

    set(path, value, callback, transactionId, scope)
    {
        return this.setMany(
            [{ path: path, value: value }],
            callback,
            transactionId,
            scope
        );
    }

    setMany(entries, callback, transactionId, scope)
    {
        return this.sendWrite(
            scope || this.scope.mode,
            null,
            entries,
            callback,
            transactionId
        );
    }

    setManyTopologyFor(instanceId, entries, callback, transactionId)
    {
        if (instanceId === undefined || instanceId === null)
        {
            throw new Error("Topology write requires an instanceId.");
        }
        return this.sendWrite("topology", instanceId, entries, callback, transactionId);
    }

    sendWrite(scope, instanceId, entries, callback, transactionId)
    {
        if (entries.length > MAX_BATCH_SIZE)
        {
            throw new Error("State batch cannot exceed 16 entries.");
        }

        let body = [String(scope)];
        if (scope === "topology")
        {
            body.push(String(instanceId));
        }
        body.push(String(transactionId || 0), entries.length);
        entries.forEach((entry) => {
            body.push("entry");
            body = body.concat(encodeStatePath(entry.path));
            body.push("value", encodeStateValue(entry.path, entry.value), "copy");
        });
        return this.protocol.request("write", body, callback);
    }

    reset(path, callback, transactionId, scope)
    {
        const frame = [String(transactionId || 0), scope || this.scope.mode]
            .concat(encodeStatePath(path));
        return this.protocol.request("reset", frame, callback);
    }

    resetTargeted(targetInstanceId, bankIndex, path, callback, transactionId, scope)
    {
        const bank = bankIndex === undefined || bankIndex === null
            ? "none"
            : Number(bankIndex);
        const frame = [
            "target",
            String(targetInstanceId),
            bank,
            String(transactionId || 0),
            scope || this.scope.mode
        ].concat(encodeStatePath(path));
        return this.protocol.request("reset", frame, callback);
    }

    handleResponseDone(args)
    {
        const completion = this.responseAssembler.finish(args);
        if (completion)
        {
            this.protocol.complete(completion.requestId, completion.result);
        }
    }

    destroy()
    {
        this.unsubscribeProtocol.forEach((unsubscribe) => unsubscribe());
        this.unsubscribeProtocol = [];
        this.responseAssembler.clear();
        this.protocol = null;
        this.scope = null;
    }
}

module.exports = {
    StateClient: StateClient
};
