function BankManagerSnapshotCoordinator(manager) {
    this.manager = manager;
}

BankManagerSnapshotCoordinator.prototype.ApplyEq = function(values) {
    var manager = this.manager;
    var state = manager.snapshotReader.ReadEq(values);
    if (!state) return false;
    var previousSelectedBankId = manager.local.selectedBankId;
    var previousLinkIds = {};
    for (var previousIndex = 0; previousIndex < manager.local.banks.length; ++previousIndex) {
        var previousLinkId = manager.local.banks[previousIndex].linkId;
        if (previousLinkId) previousLinkIds[previousIndex + 1] = previousLinkId;
    }
    manager.local.revision = state.revision;
    manager.local.eqBypass = state.bypass;
    manager.local.selectedBankId = state.selectedBankId;
    manager.local.systemBank = state.systemBank;
    manager.local.banks = state.banks;

    var changedLinkIds = {};
    var linkTopologyChanged = false;
    for (var bankIndex = 0; bankIndex < state.banks.length; ++bankIndex) {
        var previousLink = previousLinkIds[bankIndex + 1] || "";
        var currentLink = state.banks[bankIndex].linkId || "";
        if (previousLink === currentLink) continue;
        linkTopologyChanged = true;
        if (currentLink) changedLinkIds[currentLink] = true;
    }
    if (linkTopologyChanged) {
        manager.RebuildProcessorLinkGroups();
        if (manager.selection.focusedInstanceId === manager.instanceId) {
            manager.controlLinkSession = "";
            manager.RefreshControlLinkSession();
        }
    }
    if ((!manager.selection.focusedInstanceId ||
        manager.selection.focusedInstanceId === manager.instanceId) &&
        (!manager.hasCanonicalEqSnapshot || previousSelectedBankId !== state.selectedBankId)) {
        manager.SetFocusedBank(manager.local, state.selectedBankId);
    }
    manager.hasCanonicalEqSnapshot = true;
    manager.PublishAnnouncement();
    manager.PublishLinkedState(changedLinkIds);
    if (manager.pendingLinkedStatePublish) {
        manager.pendingLinkedStatePublish = false;
        manager.PublishLinkedState();
    }
    var activeLinkId = manager.ActiveLinkId(manager.local);
    manager.PublishLinkedFilterPreviews(activeLinkId,
        activeLinkId && manager.LinkMemberIds(activeLinkId).length >= 2);
    return true;
};

BankManagerSnapshotCoordinator.prototype.ApplyProcessor = function(values) {
    var manager = this.manager;
    var processors = manager.snapshotReader.ReadProcessor(values);
    if (!processors) return false;
    var shouldAnnounce = !isFinite(manager.local.processors.input_gain.values.gain);
    for (var device in processors) {
        if (!processors.hasOwnProperty(device)) continue;
        manager.local.processors[device].values = processors[device].values;
    }
    if (shouldAnnounce) {
        manager.controlLinkSession = "";
        manager.PublishAnnouncement();
        manager.PublishLinkedState();
    }
    return true;
};
