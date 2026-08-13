include("ControlBinding.js");

function BankManagerControlBinding(controller, presenter, sendMessage) {
    ControlBinding.call(this, presenter, sendMessage);
    this.controller = controller;
    this.connectPresentation();
}

BankManagerControlBinding.prototype = Object.create(ControlBinding.prototype);
BankManagerControlBinding.prototype.constructor = BankManagerControlBinding;

BankManagerControlBinding.prototype.applyPresentation = function (presentation) {
    this.send("presentation", [JSON.stringify(presentation)]);
};

BankManagerControlBinding.prototype.handleIntent = function (name, values) {
    this.controller.handleIntent(name, values);
};
