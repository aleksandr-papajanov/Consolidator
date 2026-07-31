include("../Core/ControlState.js");

function ButtonGroupViewModel() {}

ButtonGroupViewModel.prototype.BuildStates = function(
    buttons,
    labels,
    loadingIndex,
    enabled,
    selectionMode,
    pressedIndex,
    visualStates
) {
    var states = [];
    for (var index = 0; index < buttons.length; index++) {
        var state = new ControlState();
        state.enabled = enabled instanceof Array ? Boolean(enabled[index]) : enabled;
        state.active = buttons[index].IsActive()
            || (selectionMode === "custom" && pressedIndex === index);
        if (visualStates && visualStates[index]) {
            var visualState = visualStates[index];
            if (visualState.active !== undefined) state.active = visualState.active;
            state.fillColor = visualState.fillColor;
            state.borderColor = visualState.borderColor;
            state.textColor = visualState.textColor;
        }
        state.loading = loadingIndex === index + 1;
        state.label = state.loading ? "Loading" : String(labels[index]);
        states.push(state);
    }
    return states;
};
