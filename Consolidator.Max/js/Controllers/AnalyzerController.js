function AnalyzerController(presenter) {
    this.presenter = presenter;
}

AnalyzerController.prototype.handle = function (intent, payload) {
    if (!this.presenter) return;
    switch (intent) {
    case "filterMoved":
        this.presenter.filterMoved(payload[0], payload[1], payload[2]);
        break;
    case "filterQChanged":
        this.presenter.filterQChanged(payload[0], payload[1]);
        break;
    case "filterSelected":
        this.presenter.selectFilter(payload[0]);
        break;
    case "gestureBegan":
        this.presenter.beginGesture(payload[0]);
        break;
    case "gestureEnded":
        this.presenter.endGesture(payload[0]);
        break;
    }
};
