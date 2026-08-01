function BankManagerUiController(manager) {
    this.manager = manager;
}

BankManagerUiController.prototype.Paint = function() {
    this.manager.renderer.Paint(this.manager, mgraphics.size[0], mgraphics.size[1]);
};

BankManagerUiController.prototype.Scroll = function(delta) {
    var manager = this.manager;
    var step = Number(delta);
    if (!isFinite(step) || step === 0) return;
    manager.viewModel.listView.SetItems(manager.Rows());
    manager.viewModel.listView.Scroll(
        step,
        BankManagerVisualOptions.rowHeight,
        manager.ContentHeight()
    );
    mgraphics.redraw();
};

BankManagerUiController.prototype.Click = function(x, y) {
    var manager = this.manager;
    var options = BankManagerVisualOptions;
    if (manager.IsPointInRect(x, y, manager.LinkEditRect(mgraphics.size[0]))) {
        manager.ToggleLinkEditing();
        return;
    }
    if (manager.IsPointInRect(x, y, manager.ClearAllRect(mgraphics.size[0]))) {
        manager.ClearAllEqBanks();
        return;
    }
    var groupIndex = manager.LinkGroupIndexAt(x, y, mgraphics.size[0], mgraphics.size[1]);
    if (groupIndex >= 0) {
        manager.SetFocusedBankLink(
            groupIndex === 0 ? "" : manager.EditableLinkIds()[groupIndex - 1]);
        return;
    }
    var rows = manager.Rows();
    var contentHeight = manager.ContentHeight();
    var rowIndex = Math.floor((y - options.padding + manager.viewModel.listView.scrollOffset) /
        options.rowHeight);
    if (y < options.padding || y >= contentHeight + options.padding ||
        rowIndex < 0 || rowIndex >= rows.length) return;
    var instance = rows[rowIndex];
    if (x < manager.BankStartX(mgraphics.size[0])) {
        manager.SetFocusedBank(instance, instance.selectedBankId);
        mgraphics.redraw();
        return;
    }
    var displayedBanks = [instance.systemBank].concat(instance.banks);
    var squareY = Math.floor(options.padding + rowIndex * options.rowHeight -
        manager.viewModel.listView.scrollOffset +
        (options.rowHeight - options.squareSize) * 0.5);
    var bankIndex = bankGroupLayout.IndexAt(
        {
            x: manager.BankStartX(mgraphics.size[0]),
            y: squareY,
            width: manager.BankColumnWidth(mgraphics.size[0]),
            height: options.squareSize
        },
        displayedBanks.length,
        x,
        y,
        BankManagerButtonGroupOptions.banks
    );
    if (bankIndex < 0 || bankIndex >= displayedBanks.length) return;
    var bank = displayedBanks[bankIndex];
    if (bank.id === 0) return;
    if (instance.id === manager.instanceId) {
        manager.SetFocusedBank(instance, bank.id);
        manager.SendHostCommand("eq.select_bank", [bank.id]);
    } else if (manager.linkEditingEnabled && manager.ActiveEditableLinkId()) {
        manager.ToggleBankInActiveGroup(instance, bank);
    } else {
        manager.SetFocusedBank(instance, bank.id);
    }
    mgraphics.redraw();
};
