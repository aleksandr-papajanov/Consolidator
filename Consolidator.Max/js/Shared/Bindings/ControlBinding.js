class ControlBinding
{
    constructor(presenter, sendMessage)
    {
        this.presenter = presenter;
        this.sendMessage = sendMessage;
        this.unsubscribers = [];
        this.destroyed = false;
        this.presentationActive = true;
        this.pendingPresentation = null;
        this.batchSuspended = false;
        this.hasPresentation = false;
    }
    
    connectPresentation()
    {
        if (!this.presenter || typeof this.presenter.subscribe !== "function") {
            return;
        }
        this.unsubscribers.push(this.presenter.subscribe((presentation) => {
            this.receivePresentation(presentation);
        }, true));
    }
    
    suspend()
    {
        if (!this.destroyed) {
            this.batchSuspended = true;
        }
    }
    
    resumeLatest()
    {
        if (this.destroyed) {
            return;
        }
        this.batchSuspended = false;
        if (this.presentationActive) {
            this.refreshPresentation();
        }
    }
    
    receivePresentation(presentation)
    {
        if (!this.presentationActive || this.batchSuspended) {
            this.pendingPresentation = presentation;
            return;
        }
        this.applyPresentation(presentation);
        this.hasPresentation = true;
    }
    
    setPresentationActive(active)
    {
        let next = Boolean(active);
        if (this.destroyed || this.presentationActive === next) {
            return;
        }
        this.presentationActive = next;
        if (next) {
            this.refreshPresentation();
        }
    }
    
    refreshPresentation()
    {
        let presentation = this.pendingPresentation ||
            (this.presenter && this.presenter.presentation);
        this.pendingPresentation = null;
        this.hasPresentation = false;
        if (presentation) {
            this.applyPresentation(presentation);
        }
    }
    
    send(selector, args)
    {
        if (typeof this.sendMessage !== "function") {
            return;
        }
        this.sendMessage(selector, args || []);
    }
    
    handleIntent()
    {
    }
    
    destroy()
    {
        if (this.destroyed) {
            return;
        }
        this.destroyed = true;
        this.unsubscribers.forEach((unsubscribe) => { unsubscribe(); });
        this.unsubscribers = [];
        this.pendingPresentation = null;
        this.presenter = null;
        this.sendMessage = null;
    }
}

module.exports = {
    ControlBinding: ControlBinding
};

