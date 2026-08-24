function HistoryPresentation() {
    this.entries = [];
    this.cursor = 0;
    this.undo = { enabled: false, label: "Undo" };
    this.redo = { enabled: false, label: "Redo" };
}
