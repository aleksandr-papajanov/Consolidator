function BankManagerLayout() {}

BankManagerLayout.prototype.BankColumnWidth = function(width) {
    var options = BankManagerVisualOptions;
    return Math.min(options.bankColumnWidth, Math.max(1,
        width - options.padding * 2 - options.linkPanelWidth - options.columnGap));
};

BankManagerLayout.prototype.BankStartX = function(width) {
    var options = BankManagerVisualOptions;
    return Math.max(options.padding, width - options.linkPanelWidth -
        this.BankColumnWidth(width) - options.columnGap - options.padding);
};

BankManagerLayout.prototype.ContentHeight = function(height) {
    return Math.max(0, Number(height) - BankManagerVisualOptions.padding * 2);
};

BankManagerLayout.prototype.EditableLinkIds = function() {
    var ids = [];
    for (var index = 1; index <= BankManagerVisualOptions.linkGroupCount; ++index) {
        ids.push("group." + String(index));
    }
    return ids;
};

BankManagerLayout.prototype.LinkColor = function(linkId) {
    if (String(linkId) === "global.6") return InterfaceTheme.colors.secondaryAccent;
    var hash = 0;
    var value = String(linkId);
    for (var index = 0; index < value.length; ++index) {
        hash = ((hash << 5) - hash) + value.charCodeAt(index);
    }
    return BankManagerColors.linkColors[
        Math.abs(hash) % BankManagerColors.linkColors.length
    ];
};

BankManagerLayout.prototype.LinkPanelRect = function(width, height) {
    var options = BankManagerVisualOptions;
    return {
        x: Math.max(0, width - options.linkPanelWidth),
        y: options.padding + options.linkEditHeight + options.clearAllHeight + options.linkPanelGap * 2,
        width: options.linkPanelWidth,
        height: Math.max(1, height - options.padding * 2 - options.linkEditHeight -
            options.clearAllHeight - options.linkPanelGap * 2)
    };
};

BankManagerLayout.prototype.ClearAllRect = function(width) {
    var options = BankManagerVisualOptions;
    return { x: Math.max(0, width - options.linkPanelWidth),
        y: options.padding + options.linkEditHeight + options.linkPanelGap,
        width: options.linkPanelWidth, height: options.clearAllHeight };
};

BankManagerLayout.prototype.LinkEditRect = function(width) {
    var options = BankManagerVisualOptions;
    return { x: Math.max(0, width - options.linkPanelWidth), y: options.padding,
        width: options.linkPanelWidth, height: options.linkEditHeight };
};

BankManagerLayout.prototype.Contains = function(x, y, rect) {
    return x >= rect.x && x <= rect.x + rect.width &&
        y >= rect.y && y <= rect.y + rect.height;
};
