class AnalyzerGestureSession
{
    constructor(controller, transactions, send)
    {
        this.controller = controller;
        this.transactions = transactions;
        this.send = send;
        this.activeTransactionId = null;
        this.transactionReady = false;
        this.pendingEnd = false;
        this.lastMove = null;
        this.active = false;
    }

    handleIntent(name, values)
    {
        if (name === "filterReset")
        {
            this.controller.handle(name, values, null);
            return;
        }
        if (name === "gestureBegan")
        {
            this.active = true;
            this.controller.handle(name, values, this.activeTransactionId);
            this.beginTransaction();
            return;
        }
        if (name === "gestureEnded")
        {
            if (this.activeTransactionId !== null && !this.transactionReady)
            {
                this.pendingEnd = true;
            }
            else
            {
                this.commitAndFinish();
            }
            return;
        }
        if (name === "filterMoved")
        {
            this.lastMove = values.slice();
            if (this.activeTransactionId !== null && !this.transactionReady)
            {
                return;
            }
        }
        this.controller.handle(name, values, this.activeTransactionId);
    }

    beginTransaction()
    {
        if (!this.transactions || this.activeTransactionId !== null)
        {
            return;
        }
        this.transactionReady = false;
        this.pendingEnd = false;
        this.activeTransactionId = this.transactions.begin((id, response) => {
            this.completeTransactionBegin(id, response);
        });
    }

    completeTransactionBegin(id, response)
    {
        if (!this.transactions || this.activeTransactionId !== id)
        {
            return;
        }
        if (!response || response.status !== "accepted")
        {
            this.endPreviewGesture(id);
            this.send("transactionRejected");
            this.clearTransaction();
            return;
        }

        this.transactionReady = true;
        if (this.lastMove)
        {
            this.controller.handle("filterMoved", this.lastMove, id);
        }
        if (this.pendingEnd)
        {
            this.commitAndFinish();
        }
    }

    commitAndFinish()
    {
        if (!this.lastMove || !this.controller || this.activeTransactionId === null)
        {
            this.finishTransaction();
            return;
        }

        const transactionId = this.activeTransactionId;
        this.controller.handle(
            "filterCommit",
            this.lastMove.slice(),
            transactionId,
            (response) => {
                const failed = !response || response.error ||
                    response.status !== "accepted";
                const mayReset = this.activeTransactionId === null ||
                    this.activeTransactionId === transactionId;
                if (failed && mayReset)
                {
                    this.send("transactionRejected");
                }
            }
        );
        this.finishTransaction();
    }

    finishTransaction()
    {
        this.endPreviewGesture(this.activeTransactionId);
        if (this.transactions && this.activeTransactionId !== null &&
                this.transactionReady)
        {
            this.transactions.end(this.activeTransactionId);
        }
        this.clearTransaction();
    }

    endPreviewGesture(transactionId)
    {
        if (!this.active)
        {
            return;
        }
        this.active = false;
        if (this.controller)
        {
            this.controller.handle("gestureEnded", [], transactionId);
        }
    }

    cancel()
    {
        this.endPreviewGesture(this.activeTransactionId);
        if (this.transactions && this.activeTransactionId !== null &&
                this.transactionReady)
        {
            this.transactions.end(this.activeTransactionId);
        }
        this.clearTransaction();
        this.send("interactionReset");
    }

    clearTransaction()
    {
        this.activeTransactionId = null;
        this.transactionReady = false;
        this.pendingEnd = false;
        this.lastMove = null;
    }

    destroy()
    {
        this.finishTransaction();
        this.controller = null;
        this.transactions = null;
        this.send = () => {};
    }
}

module.exports = {
    AnalyzerGestureSession: AnalyzerGestureSession
};
