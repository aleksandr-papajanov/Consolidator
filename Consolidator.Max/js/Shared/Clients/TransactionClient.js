class TransactionClient
{
    constructor(protocol)
    {
        this.protocol = protocol;
        this.nextTransactionId = 1;
        this.history = {
            cursor: 0,
            entryCount: 0,
            canUndo: false,
            canRedo: false
        };
        this.listeners = [];
        this.unsubscribeHistory = protocol.on("history_state", (args) => {
            this.handleHistoryState(args);
        });
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

    jumpHistory(cursor, callback)
    {
        let target = Number(cursor);
        return this.protocol.request("jump_history", [target], callback);
    }
    
    handleHistoryState(args)
    {
        this.history = {
            cursor: Number(args[2]),
            entryCount: Number(args[3]),
            canUndo: Number(args[4]) !== 0,
            canRedo: Number(args[5]) !== 0
        };
        this.listeners.slice().forEach((listener) => {
            listener(this.history);
        });
    }

    subscribeHistory(callback, immediate)
    {
        this.listeners.push(callback);
        if (immediate) {
            callback(this.history);
        }
        return () => {
            this.listeners = this.listeners.filter((listener) => {
                return listener !== callback;
            });
        };
    }
    
    destroy()
    {
        this.unsubscribeHistory();
        this.unsubscribeHistory = () => {};
        this.protocol = null;
        this.listeners = [];
    }
}

module.exports = {
    TransactionClient: TransactionClient
};
