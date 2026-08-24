function HistoryViewModel(transactions) {
    this.transactions = transactions;
    this.state = undefined;
    this.listeners = [];
    this.unsubscribeHistory = transactions.subscribeHistory(function (state) {
        this.state = state;
        this.notify();
    }.bind(this), true);
}

HistoryViewModel.prototype.subscribe = function (callback, immediate) {
    this.listeners.push(callback);
    if (immediate && this.state) {
        callback(this.state);
    }
    var self = this;
    return function () {
        self.listeners = self.listeners.filter(function (listener) {
            return listener !== callback;
        });
    };
};

HistoryViewModel.prototype.notify = function () {
    this.listeners.slice().forEach(function (listener) {
        listener(this.state);
    }, this);
};

HistoryViewModel.prototype.destroy = function () {
    if (this.unsubscribeHistory) {
        this.unsubscribeHistory();
        this.unsubscribeHistory = null;
    }
    this.transactions = null;
    this.state = undefined;
    this.listeners = [];
};
