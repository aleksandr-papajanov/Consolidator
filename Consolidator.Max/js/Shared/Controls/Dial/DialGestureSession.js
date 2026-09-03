class DialGestureSession
{
    constructor(presenter, transactions, reject)
    {
        this.presenter = presenter;
        this.transactions = transactions;
        this.reject = reject;
        this.transactionId = null;
        this.ready = false;
        this.pendingValue = null;
        this.pendingEnd = null;
    }

    begin(index)
    {
        if (this.transactions) {
            this.clear();
            this.transactionId = this.transactions.begin(
                this.accept.bind(this));
        }
        this.presenter.beginGesture(index, this.transactionId);
    }

    setValue(index, value)
    {
        if (this.transactionId !== null && !this.ready) {
            this.pendingValue = [index, value];
            return;
        }
        this.presenter.setValue(index, value, this.transactionId);
    }

    end(index)
    {
        if (this.transactions && this.transactionId !== null) {
            if (!this.ready) {
                this.pendingEnd = index;
                return;
            }
            this.presenter.endGesture(index, this.transactionId);
            this.finish();
            return;
        }
        this.presenter.endGesture(index, null);
    }

    accept(id, response)
    {
        if (!this.transactions || this.transactionId !== id) return;
        if (!response || response.status !== "accepted") {
            this.reject();
            this.clear();
            return;
        }
        this.ready = true;
        if (this.pendingValue) {
            this.presenter.setValue(
                this.pendingValue[0], this.pendingValue[1], id);
            this.pendingValue = null;
        }
        if (this.pendingEnd !== null) {
            this.presenter.endGesture(this.pendingEnd, id);
            this.finish();
        }
    }

    finish()
    {
        this.transactions.end(this.transactionId);
        this.clear();
    }

    clear()
    {
        this.transactionId = null;
        this.ready = false;
        this.pendingValue = null;
        this.pendingEnd = null;
    }

    destroy()
    {
        if (this.transactions && this.transactionId !== null && this.ready) {
            this.transactions.end(this.transactionId);
        }
        this.clear();
        this.transactions = null;
        this.presenter = null;
    }
}

module.exports = {
    DialGestureSession: DialGestureSession
};
