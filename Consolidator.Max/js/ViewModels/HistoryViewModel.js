class HistoryViewModel
{
    constructor(transactions)
    {
        this.transactions = transactions;
        this.state = undefined;
        this.listeners = [];
        this.unsubscribeHistory = transactions.subscribeHistory((state) => {
            this.state = state;
            this.notify();
        }.bind(this), true);
    }
    
    subscribe(callback, immediate)
    {
        this.listeners.push(callback);
        if (immediate && this.state) {
            callback(this.state);
        }
        return () => {
            this.listeners = this.listeners.filter((listener) => {
                return listener !== callback;
            });
        };
    }
    
    notify()
    {
        this.listeners.slice().forEach((listener) => {
            listener(this.state);
        }, this);
    }
    
    destroy()
    {
        if (this.unsubscribeHistory) {
            this.unsubscribeHistory();
            this.unsubscribeHistory = null;
        }
        this.transactions = null;
        this.state = undefined;
        this.listeners = [];
    }
}


module.exports = {
    HistoryViewModel: HistoryViewModel
};
