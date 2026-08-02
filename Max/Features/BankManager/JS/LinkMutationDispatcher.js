function LinkMutationDispatcher(manager) {
    this.manager = manager;
    this.pending = {};
    this.order = [];
    this.scheduled = false;
    this.delivery = new Task(this.DeliverNext, this);
}

LinkMutationDispatcher.prototype.Enqueue = function(instance, bank, linkId) {
    if (!instance || !bank || bank.id < 2 || bank.id > 5) return;
    var desiredLinkId = String(linkId || "");
    var key = String(instance.id) + ":" + String(bank.id);
    var pending = this.pending[key];
    if (pending) {
        pending.linkId = desiredLinkId;
        return;
    }
    if (String(bank.linkId || "") === desiredLinkId) return;
    this.pending[key] = {
        instanceId: String(instance.id),
        bankId: Number(bank.id),
        linkId: desiredLinkId,
        previousLinkId: String(bank.linkId || "")
    };
    this.order.push(key);
    if (this.scheduled) return;
    this.scheduled = true;
    this.delivery.schedule(0);
};

LinkMutationDispatcher.prototype.DeliverNext = function() {
    this.scheduled = false;
    if (!this.order.length) return;
    var key = this.order.shift();
    var mutation = this.pending[key];
    delete this.pending[key];
    if (mutation) this.Deliver(mutation);
    if (!this.order.length) return;
    this.scheduled = true;
    this.delivery.schedule(0);
};

LinkMutationDispatcher.prototype.Deliver = function(mutation) {
    var manager = this.manager;
    if (mutation.instanceId === manager.instanceId) {
        manager.SendHostCommand("eq.set_link", [mutation.bankId, mutation.linkId || "-"]);
        outlet(1, "coordinator.changed");
        return;
    }
    if (mutation.linkId) {
        outlet(1, "link.assign", mutation.linkId, mutation.instanceId, mutation.bankId);
        outlet(1, "coordinator.changed");
        return;
    }
    outlet(1, "link.detach", mutation.previousLinkId, mutation.instanceId, mutation.bankId);
    outlet(1, "coordinator.changed");
};

LinkMutationDispatcher.prototype.Dispose = function() {
    this.delivery.cancel();
    this.pending = {};
    this.order = [];
    this.scheduled = false;
};
