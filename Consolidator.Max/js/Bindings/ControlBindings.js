include("Project:/js/Bindings/DialControlBinding.js");
include("Project:/js/Bindings/ButtonControlBinding.js");
include("Project:/js/Bindings/AnalyzerControlBinding.js");
include("Project:/js/Bindings/BankManagerControlBinding.js");

function ControlBindings() {
    this.items = {};
    this.presentationActive = true;
}

ControlBindings.prototype.add = function (name, binding) {
    if (!name) throw new Error("Control binding requires a varname.");
    if (this.items.hasOwnProperty(name)) {
        throw new Error("Duplicate control binding varname: " + name);
    }
    if (binding) {
        binding.setPresentationActive(this.presentationActive);
        this.items[name] = binding;
    }
    return binding;
};

ControlBindings.prototype.setPresentationActive = function (active) {
    this.presentationActive = Boolean(active);
    Object.keys(this.items).forEach(function (name) {
        this.items[name].setPresentationActive(this.presentationActive);
    }, this);
};

ControlBindings.prototype.handle = function (name, intent, values) {
    var binding = this.items[name];
    if (binding) {
        binding.handleIntent(intent, values || []);
    }
};

ControlBindings.prototype.destroy = function () {
    Object.keys(this.items).forEach(function (name) {
        this.items[name].destroy();
    }, this);
    this.items = {};
    this.presentationActive = false;
};
