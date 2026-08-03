include("../Core/ControlRenderer.js");
include("../Button/ButtonViewModel.js");
include("../ButtonGroup/ButtonGroupLayout.js");
include("../ButtonGroup/ButtonGroupViewModel.js");
include("../ButtonGroup/ButtonGroupRenderer.js");
include("../List/ListRenderer.js");

var BankManagerButtonGroupOptions = {
    banks: CreateButtonGroupOptions({
        layout: "horizontal",
        selectionMode: "multiple",
        sizing: "equal"
    }),
    links: CreateButtonGroupOptions({
        layout: "vertical",
        selectionMode: "single",
        sizing: "equal"
    }),
    action: CreateButtonGroupOptions({
        layout: "horizontal",
        selectionMode: "custom",
        sizing: "equal"
    })
};

function BankManagerRenderer() {
    this.controlRenderer = new ControlRenderer();
    this.buttonGroupRenderer = new ButtonGroupRenderer();
    this.listRenderer = new ListRenderer();
}

BankManagerRenderer.prototype.WithAlpha = function(color, alpha) {
    return [color[0], color[1], color[2], alpha];
};

BankManagerRenderer.prototype.CreateGroup = function(labels, modes, enabled, visualStates, options) {
    var group = {
        viewModel: new ButtonGroupViewModel(),
        buttons: [],
        labels: labels,
        loadingIndex: 0,
        enabled: enabled,
        visualStates: visualStates,
        options: options,
        pressedIndex: -1
    };
    for (var index = 0; index < labels.length; ++index) {
        var button = new ButtonViewModel(modes[index] || "toggle");
        button.SetValue(visualStates[index] && visualStates[index].active ? 1 : 0);
        group.buttons.push(button);
    }
    return group;
};

BankManagerRenderer.prototype.DrawBanks = function(manager, instance, banks, x, y, local, width) {
    var colors = BankManagerColors;
    var labels = [];
    var enabled = [];
    var visualStates = [];
    for (var index = 0; index < banks.length; ++index) {
        var bank = banks[index];
        var interactive = bank.id !== 0;
        var selected = interactive && !manager.linkEditingEnabled &&
            manager.IsFocusedBank(instance, bank);
        var editSelected = manager.groupOperations.IsSelected(instance, bank);
        var visible = manager.visibilityPolicy.IsVisible(instance, bank, local);
        if (!visible) {
            labels.push("");
            enabled.push(false);
            visualStates.push({
                active: false,
                fillColor: InterfaceTheme.colors.transparent,
                borderColor: InterfaceTheme.colors.transparent,
                textColor: InterfaceTheme.colors.transparent
            });
            continue;
        }
        var color = bank.id === 0 ? colors.systemBank : colors.bankDefault;
        if (bank.linkId) color = manager.LinkColor(bank.linkId);
        var alpha = manager.visibilityPolicy.Opacity(bank, local, selected, editSelected);
        color = this.WithAlpha(color, alpha);
        labels.push(String(bank.id));
        enabled.push(manager.visibilityPolicy.IsEnabled(bank, local));
        var active = selected || editSelected;
        visualStates.push({
            active: active,
            fillColor: active ? color : InterfaceTheme.states.inactive.fill,
            borderColor: color,
            textColor: active
                ? colors.background : color
        });
    }
    var group = this.CreateGroup(
        labels,
        [],
        enabled,
        visualStates,
        BankManagerButtonGroupOptions.banks
    );
    var cells = this.buttonGroupRenderer.Cells(
        group,
        { x: x, y: y, width: Math.max(1, width), height: BankManagerVisualOptions.squareSize }
    );
    this.buttonGroupRenderer.Paint(group, cells);
};

BankManagerRenderer.prototype.DrawLinkGroups = function(manager, width, height) {
    var labels = [];
    var activeLinkId = manager.ActiveEditableLinkId();
    var enabled = [];
    var visualStates = [];
    var linkIds = manager.EditableLinkIds();
    for (var index = 0; index < linkIds.length; ++index) {
        var linkId = linkIds[index];
        labels.push(String(index + 1));
        var color = manager.LinkColor(linkId);
        var used = manager.LinkMemberCount(linkId) > 0;
        var active = manager.linkEditingEnabled
            ? manager.groupOperations.HasSelectionInLink(linkId)
            : activeLinkId === linkId;
        var assignable = manager.groupOperations.CanApplySelection(linkId);
        var interactive = manager.linkEditingEnabled && assignable;
        enabled.push(interactive);
        var visibleAlpha = manager.linkEditingEnabled
            ? interactive ? 1.0 : 0.25
            : 1.0;
        visualStates.push({
            active: active,
            borderColor: this.WithAlpha(color, visibleAlpha),
            fillColor: active ? color : used ? this.WithAlpha(color, 0.18 * visibleAlpha) : null,
            textColor: active ? BankManagerColors.background
                : this.WithAlpha(color, visibleAlpha)
        });
    }
    var group = this.CreateGroup(
        labels,
        [],
        enabled,
        visualStates,
        BankManagerButtonGroupOptions.links
    );
    this.buttonGroupRenderer.Paint(
        group,
        manager.layout.LinkGroupCells(width, height)
    );
};

BankManagerRenderer.prototype.DrawLinkEditToggle = function(manager, width) {
    var active = manager.linkEditingEnabled;
    var enabled = manager.CanChangeFocusedBankLink();
    var visualState = enabled
        ? {
            active: active,
            borderColor: InterfaceTheme.colors.secondaryAccent,
            fillColor: active ? InterfaceTheme.colors.secondaryAccent : null,
            textColor: active
                ? BankManagerColors.background
                : InterfaceTheme.colors.secondaryAccent
        }
        : { active: false };
    this.buttonGroupRenderer.Paint(
        this.CreateGroup(
            ["edit"],
            ["toggle"],
            [enabled],
            [visualState],
            BankManagerButtonGroupOptions.action
        ),
        [manager.LinkEditRect(width)]
    );
};

BankManagerRenderer.prototype.DrawClearAllButton = function(manager, width) {
    var enabled = Boolean(manager.instanceId);
    var label = manager.clearAllConfirmationArmed ? "sure?" : "clear";
    this.buttonGroupRenderer.Paint(
        this.CreateGroup(
            [label],
            ["momentary"],
            [enabled],
            [enabled
                ? {
                    active: false,
                    borderColor: InterfaceTheme.colors.secondaryAccent,
                    fillColor: null,
                    textColor: InterfaceTheme.colors.secondaryAccent
                }
                : { active: false }],
            BankManagerButtonGroupOptions.action
        ),
        [manager.ClearAllRect(width)]
    );
};

BankManagerRenderer.prototype.FitText = function(value, maximumWidth) {
    var text = String(value);
    while (text.length > 1 && mgraphics.text_measure(text)[0] > maximumWidth) {
        text = text.substring(0, text.length - 1);
    }
    return text === value ? text : text.substring(0, Math.max(1, text.length - 3)) + "...";
};

BankManagerRenderer.prototype.Paint = function(manager, width, height) {
    var options = BankManagerVisualOptions;
    var colors = BankManagerColors;
    var presentation = manager.viewModel.Build(manager, width, height);
    this.controlRenderer.SetFont(InterfaceTheme.typography.fontFamily, InterfaceTheme.typography.minimumSize);
    var bankStartX = manager.BankStartX(width);
    var trackColumnWidth = bankStartX - options.padding - options.columnGap;
    var visibleRows = this.listRenderer.VisibleRows(
        presentation.rows, presentation.scrollOffset, options.padding,
        presentation.contentHeight + options.padding, options.rowHeight);
    for (var visibleIndex = 0; visibleIndex < visibleRows.length; ++visibleIndex) {
        var row = visibleRows[visibleIndex];
        var y = row.y;
        if (y + options.rowHeight <= options.padding || y >= presentation.contentHeight + options.padding) continue;
        var instance = row.item;
        var local = instance.id === manager.instanceId;
        if (local) {
            this.controlRenderer.SetColor(colors.background);
            mgraphics.rectangle(0, y, width, options.rowHeight);
            mgraphics.fill();
        }
        this.controlRenderer.DrawCenteredText(
            this.FitText(instance.label, Math.max(1, trackColumnWidth)),
            { x: options.padding, y: y, width: Math.max(1, trackColumnWidth), height: options.rowHeight },
            local ? colors.bankDefault : colors.instanceText,
            InterfaceTheme.typography.fontFamily, InterfaceTheme.typography.minimumSize);
        this.DrawBanks(manager, instance, [instance.systemBank].concat(instance.banks),
            bankStartX, Math.floor(y + (options.rowHeight - options.squareSize) * 0.5), local,
            manager.BankColumnWidth(width));
        this.controlRenderer.SetColor(colors.separator);
        mgraphics.rectangle(options.padding, y + options.rowHeight - options.separatorWidth,
            width - options.padding * 2, options.separatorWidth);
        mgraphics.fill();
    }
    this.DrawLinkEditToggle(manager, width);
    this.DrawClearAllButton(manager, width);
    this.DrawLinkGroups(manager, width, height);
};
