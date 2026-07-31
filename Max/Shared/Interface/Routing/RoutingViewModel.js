include("../List/ListViewModel.js");

function RoutingMenuViewModel(label) {
    ListViewModel.call(this);
    this.label = label;
    this.items = ["None"];
    this.selection = 1;
    this.enabled = false;
}

RoutingMenuViewModel.prototype = Object.create(ListViewModel.prototype);
RoutingMenuViewModel.prototype.constructor = RoutingMenuViewModel;

RoutingMenuViewModel.prototype.SetItems = function(values) {
    var items = [];
    for (var index = 0; index < values.length; index++) items.push(String(values[index]));
    if (items.length === 0) items.push("None");
    ListViewModel.prototype.SetItems.call(this, items);
};

RoutingMenuViewModel.prototype.SetSelection = function(value) {
    ListViewModel.prototype.SetSelection.call(this, value);
};

RoutingMenuViewModel.prototype.SelectedText = function() {
    return this.items[this.selection - 1] || "None";
};

function RoutingViewModel() {
    this.source = new RoutingMenuViewModel("Source");
    this.channel = new RoutingMenuViewModel("Channel");
}

RoutingViewModel.prototype.State = function(name) {
    if (name === "source") return this.source;
    if (name === "channel") return this.channel;
    return null;
};
