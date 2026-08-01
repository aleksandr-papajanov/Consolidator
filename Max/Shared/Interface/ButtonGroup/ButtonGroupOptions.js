include("../../Configuration/InterfaceTheme.js");

var ButtonGroupOptions = {
    count: 3,
    layout: "horizontal",
    selectionMode: "single",
    allowEmptySelection: true,
    buttonModes: ["toggle", "toggle", "toggle"],
    paddingRatio: InterfaceTheme.controls.buttonGroup.paddingRatio,
    gapRatio: InterfaceTheme.controls.buttonGroup.gapRatio,
    contentPadding: InterfaceTheme.controls.buttonGroup.contentPadding,
    cornerRadiusRatio: InterfaceTheme.controls.buttonGroup.cornerRadiusRatio
};

function CreateButtonGroupOptions(overrides) {
    var options = {
        count: ButtonGroupOptions.count,
        layout: ButtonGroupOptions.layout,
        selectionMode: ButtonGroupOptions.selectionMode,
        allowEmptySelection: ButtonGroupOptions.allowEmptySelection,
        buttonModes: ButtonGroupOptions.buttonModes.slice(),
        paddingRatio: ButtonGroupOptions.paddingRatio,
        gapRatio: ButtonGroupOptions.gapRatio,
        contentPadding: ButtonGroupOptions.contentPadding,
        cornerRadiusRatio: ButtonGroupOptions.cornerRadiusRatio,
        sizing: "content"
    };
    if (!overrides) return options;
    for (var key in overrides) {
        if (overrides.hasOwnProperty(key)) options[key] = overrides[key];
    }
    return options;
}
