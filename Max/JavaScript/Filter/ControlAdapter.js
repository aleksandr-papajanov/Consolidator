function ControlAdapter(controlMap) {
    this.controlMap = controlMap;
}

ControlAdapter.prototype.createCommand = function(message) {
    var varname = this.controlMap[message.controlId];
    if (!varname) {
        return null;
    }

    var command = ["script", "sendbox", varname];
    if (message.action === "move") {
        return command.concat(["presentation_rect"], message.values);
    }
    if (message.action === "show") {
        return command.concat(["hidden", 0]);
    }
    if (message.action === "hide") {
        return command.concat(["hidden", 1]);
    }
    if (message.action === "enable") {
        return command.concat(["active", 1]);
    }
    if (message.action === "disable") {
        return command.concat(["active", 0]);
    }
    if (message.action === "set") {
        return command.concat(["set", message.values[0]]);
    }
    if (message.action === "color") {
        var colorAttribute = varname.indexOf(".numbox") !== -1
            ? "activeslidercolor"
            : (varname === "bypass" || varname === "reset")
                ? "lcdcolor"
                : "activedialcolor";
        return command.concat([colorAttribute], message.values);
    }
    if (message.action === "outputvalue") {
        return command.concat(["outputvalue"]);
    }
    return null;
};
