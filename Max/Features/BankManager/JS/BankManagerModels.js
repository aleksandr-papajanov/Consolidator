function BankSummary() {
    this.id = 0;
    this.occupied = false;
    this.linkId = "";
}

function InstanceSummary(id, label) {
    this.id = id;
    this.label = label;
    this.trackOrder = Infinity;
    this.revision = 0;
    this.selectedBankId = 1;
    this.systemBank = new BankSummary();
    this.banks = [];
    for (var bankId = 1; bankId <= 6; ++bankId) {
        var bank = new BankSummary();
        bank.id = bankId;
        this.banks.push(bank);
    }
}
