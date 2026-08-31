class AnalyzerController
{
    constructor(presenter)
    {
        this.presenter = presenter;
    }
    
    handle(intent, payload, transactionId, callback)
    {
        if (!this.presenter) {
            return;
        }
        switch (intent) {
        case "gestureBegan":
            this.presenter.beginPreviewGesture();
            break;
        case "gestureEnded":
            this.presenter.endPreviewGesture();
            break;
        case "filterPreview":
            this.presenter.previewMoved(payload[0], payload[1], payload[2]);
            break;
        case "filterMoved":
            this.presenter.filterMoved(
                payload[0], payload[1], payload[2], transactionId);
            break;
        case "filterCommit":
            this.presenter.commitPreview(
                payload[0], payload[1], payload[2], transactionId, callback);
            break;
        case "filterQChanged":
            this.presenter.filterQChanged(payload[0], payload[1]);
            break;
        case "filterSelected":
            this.presenter.selectFilter(payload[0]);
            break;
        case "filterReset":
            this.presenter.resetFilter(payload[0], callback);
            break;
        }
    }
}

module.exports = {
    AnalyzerController: AnalyzerController
};
