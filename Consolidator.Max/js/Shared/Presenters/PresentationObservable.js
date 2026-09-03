class PresentationObservable
{
        static batchDepth = 0;
        static batchPresenters = [];
    
        constructor()
        {
            this.listeners = [];
            this.destroyed = false;
        }
    
        static beginBatch()
        {
            PresentationObservable.batchDepth += 1;
        }
    
        static endBatch()
        {
            if (PresentationObservable.batchDepth === 0)
            {
                return;
            }
            PresentationObservable.batchDepth -= 1;
            if (PresentationObservable.batchDepth !== 0)
            {
                return;
            }
            const presenters = PresentationObservable.batchPresenters;
            PresentationObservable.batchPresenters = [];
            presenters.forEach((presenter) =>
            {
                if (!presenter.destroyed && presenter.rebuildRequested)
                {
                    presenter.rebuildRequested = false;
                    presenter.rebuild();
                }
            });
        }
    
        requestRebuild()
        {
            if (this.destroyed)
            {
                return;
            }
            if (PresentationObservable.batchDepth > 0)
            {
                if (!this.rebuildRequested)
                {
                    this.rebuildRequested = true;
                    PresentationObservable.batchPresenters.push(this);
                }
                return;
            }
            this.rebuild();
        }
    
        subscribe(callback, immediate)
        {
            if (this.destroyed)
            {
                return () => {};
            }
    
            this.listeners.push(callback);
            if (immediate && this.presentation)
            {
                callback(this.presentation);
            }
    
            return () =>
            {
                this.listeners = this.listeners.filter((listener) =>
                {
                    return listener !== callback;
                });
            };
        }
    
        publish(presentation)
        {
            if (this.destroyed)
            {
                return;
            }
    
            this.presentation = presentation;
            const listeners = this.listeners.slice();
            for (let index = 0; index < listeners.length; index += 1)
            {
                listeners[index](presentation);
            }
        }
    
        destroy()
        {
            this.destroyed = true;
            this.listeners = [];
            this.presentation = null;
        }
}

module.exports = {
    PresentationObservable: PresentationObservable
};
