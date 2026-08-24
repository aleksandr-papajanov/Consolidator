include("Project:/js/Presenters/Core/PresentationObservable.js");
include("Project:/js/Presenters/History/HistoryPresentation.js");

function HistoryPresenter(viewModel) {
    PresentationObservable.call(this);
    this.viewModel = viewModel;
    this.unsubscribeViewModel = viewModel.subscribe(function () {
        this.rebuild();
    }.bind(this), true);
    this.rebuild();
}

HistoryPresenter.prototype = Object.create(PresentationObservable.prototype);
HistoryPresenter.prototype.constructor = HistoryPresenter;

HistoryPresenter.prototype.rebuild = function () {
    var presentation = new HistoryPresentation();
    var state = this.viewModel && this.viewModel.state;
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
};

HistoryPresenter.prototype.destroy = function () {
    if (this.destroyed) {
        return;
    }
    if (this.unsubscribeViewModel) {
        this.unsubscribeViewModel();
        this.unsubscribeViewModel = null;
    }
    this.viewModel = null;
    PresentationObservable.prototype.destroy.call(this);
};
