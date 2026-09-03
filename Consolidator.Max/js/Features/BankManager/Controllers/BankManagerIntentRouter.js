function routeBankManagerIntent(controller, name, values)
{
    values = values || [];
    switch (name) {
    case "bankSelected":
        controller.selectBank(values[0], values[1], Number(values[2]) !== 0);
        break;
    case "panelSelected": controller.selectPanel(values[0]); break;
    case "processorSelected": controller.selectProcessor(values[0], values[1]); break;
    case "rowSelected": controller.selectRow(values[0]); break;
    case "groupRequested": controller.groupSelectedBanks(); break;
    case "ungroupRequested": controller.ungroupFocusedBank(); break;
    case "clearRequested": controller.clearGroups(); break;
    case "scopeToggled": controller.context.viewModel.toggleScope(); break;
    case "historySelected": controller.jumpHistory(values[0]); break;
    case "instanceSoloChanged":
        controller.setSolo(values[0], Number(values[1]) !== 0,
            Number(values[2]) !== 0);
        break;
    case "instanceMuteChanged":
        controller.setMute(values[0], Number(values[1]) !== 0,
            Number(values[2]) !== 0);
        break;
    case "instanceBypassChanged":
        controller.setBypass(values[0], Number(values[1]) !== 0,
            Number(values[2]) !== 0);
        break;
    case "instanceResetRequested": controller.resetInstance(); break;
    case "processorBypassChanged":
        if (values.length >= 3) {
            controller.setProcessorBypass(
                values[0], values[1], Number(values[2]) !== 0);
        }
        break;
    case "bankBypassChanged":
        if (values.length >= 3) {
            controller.setBankBypass(
                values[0], values[1], Number(values[2]) !== 0);
        }
        break;
    case "bankResetRequested": controller.resetBank(values[0], values[1]); break;
    case "processorResetRequested":
        controller.resetProcessor(values[0], values[1]);
        break;
    }
}

module.exports = {
    routeBankManagerIntent: routeBankManagerIntent
};
