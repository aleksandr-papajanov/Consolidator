function TransactionClient(protocol) {
    this.protocol = protocol;
    this.nextTransactionId = 1;
    this.history = { cursor: 0, entryCount: 0 };
    protocol.on("history_state", this.handleHistoryState.bind(this));
}

TransactionClient.prototype.begin = function (callback) {
    var id = this.nextTransactionId++;
    this.protocol.request("begin_history", [String(id)], function (response) {
        if (callback) {
            callback(id, response);
        }
    });
    return id;
};

TransactionClient.prototype.end = function (id) {
    return this.protocol.request("end_history", [String(id)]);
};

TransactionClient.prototype.undo = function (callback) {
    return this.protocol.request("jump_history",
        [Math.max(0, this.history.cursor - 1)], callback);
};

TransactionClient.prototype.redo = function (callback) {
    return this.protocol.request("jump_history",
        [Math.min(this.history.entryCount, this.history.cursor + 1)], callback);
};

TransactionClient.prototype.handleHistoryState = function (args) {
    this.history = { cursor: Number(args[2]), entryCount: Number(args[3]) };
};

TransactionClient.prototype.destroy = function () {
    this.protocol = null;
};
