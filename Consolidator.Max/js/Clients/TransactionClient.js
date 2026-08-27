class TransactionClient
{
    constructor(protocol)
    {
        this.protocol = protocol;
        this.nextTransactionId = 1;
        this.history = { cursor: 0, entryCount: 0 };
        protocol.on("history_state", this.handleHistoryState.bind(this));
    }
    
    begin(callback)
    {
        let id = this.nextTransactionId++;
        this.protocol.request("begin_history", [String(id)], (response) => {
            if (callback) {
                callback(id, response);
            }
        });
        return id;
    }
    
    end(id)
    {
        return this.protocol.request("end_history", [String(id)]);
    }
    
    undo(callback)
    {
        return this.protocol.request("jump_history",
            [Math.max(0, this.history.cursor - 1)], callback);
    }
    
    redo(callback)
    {
        return this.protocol.request("jump_history",
            [Math.min(this.history.entryCount, this.history.cursor + 1)], callback);
    }
    
    handleHistoryState(args)
    {
        this.history = { cursor: Number(args[2]), entryCount: Number(args[3]) };
    }
    
    destroy()
    {
        this.protocol = null;
    }
}

module.exports = {
    TransactionClient: TransactionClient
};
