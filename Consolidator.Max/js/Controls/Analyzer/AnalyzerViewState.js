class AnalyzerViewState
{
    constructor()
    {
        this.selectedId = 0;
        this.dragging = false;
        this.preview = {};
        this.pendingMove = null;
    }
}

module.exports = {
    AnalyzerViewState: AnalyzerViewState
};
