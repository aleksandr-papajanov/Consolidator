include("../../Configuration/InterfaceTheme.js");

var BankManagerOptions = {
    colors: {
        background: InterfaceTheme.colors.background,
        instanceText: InterfaceTheme.colors.textMuted,
        separator: InterfaceTheme.colors.borderInactive,
        bankDefault: InterfaceTheme.colors.primaryAccent,
        linkColors: InterfaceTheme.colors.linkPalette,
        systemBank: InterfaceTheme.colors.textMuted,
        inactiveBank: InterfaceTheme.colors.textInactive,
        disabledText: InterfaceTheme.colors.textDisabled
    },
    geometry: {
        padding: 0,
        rowHeight: 17,
        bankColumnWidth: 120,
        linkPanelWidth: 34,
        linkEditHeight: 16,
        clearAllHeight: 16,
        clearAllConfirmTimeoutMs: 3000,
        linkPanelGap: 2,
        columnGap: 0,
        squareSize: 16,
        focusedBankOutlineWidth: 2,
        separatorWidth: 1,
        linkGroupCount: 10,
        inactiveBankOpacity: 0.45
    }
};
