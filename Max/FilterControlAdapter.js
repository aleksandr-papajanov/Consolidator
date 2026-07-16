autowatch = 1;
inlets = 1;
outlets = 1;

var controls = {
    gain: "gain_dial",
    frequency: "freq.numbox",
    q: "q.numbox",
    bypass: "bypass",
    reset: "reset"
};

function control() {
    var args = arrayfromargs(arguments);
    if (args.length < 2) {
        return;
    }

    var varname = controls[args[0]];
    if (!varname) {
        return;
    }

    var action = String(args[1]);
    var command = ["script", "sendbox", varname];

    if (action === "move" && args.length === 6) {
        command.push("presentation_rect", args[2], args[3], args[4], args[5]);
    }
    else if (action === "show") {
        command.push("hidden", 0);
    }
    else if (action === "hide") {
        command.push("hidden", 1);
    }
    else if (action === "enable") {
        command.push("active", 1);
    }
    else if (action === "disable") {
        command.push("active", 0);
    }
    else if (action === "set" && args.length === 3) {
        command.push("set", args[2]);
    }
    else if (action === "color" && args.length === 6) {
        var colorAttribute = varname.indexOf(".numbox") !== -1
            ? "activeslidercolor"
            : (varname === "bypass" || varname === "reset")
                ? "lcdcolor"
                : "activedialcolor";
        command.push(colorAttribute, args[2], args[3], args[4], args[5]);
    }
    else if (action === "outputvalue") {
        command.push("outputvalue");
    }
    else {
        return;
    }

    outlet(0, command);
}
