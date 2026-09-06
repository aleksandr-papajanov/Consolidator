class AnalyzerViewState
{
    constructor()
    {
        this.selectedId = 0;
        this.dragging = false;
        this.pendingMove = null;
        this.lastDragY = 0;
        this.qDragStarted = false;
    }
}

module.exports = {
    AnalyzerViewState: AnalyzerViewState
};
