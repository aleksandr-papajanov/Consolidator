class EditingActionClient
{
    constructor(protocol)
    {
        this.protocol = protocol;
    }
    
    joinBanks(callback)
    {
        return this.protocol.request("join_banks", [], callback);
    }
    
    clearBanks(callback)
    {
        return this.protocol.request("clear_banks", [], callback);
    }
    
    linkGroup(linkId, banks, callback)
    {
        let body = [String(linkId), banks.length];
        banks.forEach((bank) => {
            body.push(String(bank.instanceId), Number(bank.bankId));
        });
        return this.protocol.request("apply_link_group", body, callback);
    }
    
    destroy()
    {
        this.protocol = null;
    }
}

module.exports = {
    EditingActionClient: EditingActionClient
};
