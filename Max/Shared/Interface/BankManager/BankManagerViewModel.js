include("../List/ListViewModel.js");

function BankManagerViewModel() {
    this.listView = new ListViewModel();
}

BankManagerViewModel.prototype.Build = function(manager, width, height) {
    var options = BankManagerVisualOptions;
    this.listView.SetItems(manager.Rows());
    var viewportHeight = Math.max(0, height - options.padding * 2);
    var scrollOffset = this.listView.ClampScroll(viewportHeight, options.rowHeight);
    return {
        width: width,
        height: height,
        rows: this.listView.items,
        contentHeight: viewportHeight,
        scrollOffset: scrollOffset,
        activeLinkId: manager.ActiveEditableLinkId(),
        focusedInstanceId: manager.FocusedInstance().id,
        focusedBankId: manager.focusedBankId
    };
};
