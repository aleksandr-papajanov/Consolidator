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
        this.pendingQ = {};
        this.active = false;
        this.gestureGeneration = 0;
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
            this.gestureGeneration += 1;
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
        if (name === "filterQChanged" && this.activeTransactionId !== null &&
                !this.transactionReady)
        {
            this.controller.handle("filterQPreview", values, null);
            this.pendingQ[String(values[0])] = true;
            return;
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
        Object.keys(this.pendingQ).forEach((filterId) =>
        {
            this.controller.handle("filterQCommit", [Number(filterId)], id);
        });
        this.pendingQ = {};
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
        const gestureGeneration = this.gestureGeneration;
        this.controller.handle(
            "filterCommit",
            this.lastMove.slice(),
            null,
            (response) =>
            {
                if (this.gestureGeneration !== gestureGeneration)
                {
                    return;
                }

                const failed = !response || Boolean(response.error) || response.status !== "accepted";
                this.endPreviewGesture(transactionId);
                if (failed)
                {
                    this.send("transactionRejected");
                }
            }
        );
        if (this.transactions && this.transactionReady)
        {
            this.transactions.end(transactionId);
        }
        this.clearTransaction();
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
        this.gestureGeneration += 1;
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
        this.pendingQ = {};
    }

    destroy()
    {
        this.gestureGeneration += 1;
        this.finishTransaction();
        this.controller = null;
        this.transactions = null;
        this.send = () => {};
    }
}

module.exports = {
    AnalyzerGestureSession: AnalyzerGestureSession
};
