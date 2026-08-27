const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { HistoryPresentation } = require("./HistoryPresentation.js");

class HistoryPresenter extends PresentationObservable
{
    constructor(viewModel)
    {
        super();
        this.viewModel = viewModel;
        this.unsubscribeViewModel = viewModel.subscribe(() => {
            this.rebuild();
        }.bind(this), true);
        this.rebuild();
    }
    
    rebuild()
    {
        let presentation = new HistoryPresentation();
        let state = this.viewModel && this.viewModel.state;
        if (state) {
            presentation.entries = state.entries || [];
            presentation.cursor = state.cursor || 0;
        }
        if (state && state.undo) {
            presentation.undo = {
                enabled: true,
                label: "Undo: " + state.undo.label
            };
        }
        if (state && state.redo) {
            presentation.redo = {
                enabled: true,
                label: "Redo: " + state.redo.label
            };
        }
        this.publish(presentation);
    }
    
    destroy()
    {
        if (this.destroyed) {
            return;
        }
        if (this.unsubscribeViewModel) {
            this.unsubscribeViewModel();
            this.unsubscribeViewModel = null;
        }
        this.viewModel = null;
        super.destroy();
    }
}


module.exports = {
    HistoryPresenter: HistoryPresenter
};
