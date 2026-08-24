function EditingActionClient(protocol) {
    this.protocol = protocol;
}

EditingActionClient.prototype.joinBanks = function (callback) {
    return this.protocol.request("join_banks", [], callback);
};

EditingActionClient.prototype.clearBanks = function (callback) {
    return this.protocol.request("clear_banks", [], callback);
};

EditingActionClient.prototype.linkGroup = function (linkId, banks, callback) {
    var body = [String(linkId), banks.length];
    banks.forEach(function (bank) {
        body.push(String(bank.instanceId), Number(bank.bankId));
    });
    return this.protocol.request("apply_link_group", body, callback);
};

EditingActionClient.prototype.destroy = function () {
    this.protocol = null;
};
