const { DialControlBinding } = require("./DialControlBinding.js");
const { ButtonControlBinding } = require("./ButtonControlBinding.js");
const { AnalyzerControlBinding } = require("./AnalyzerControlBinding.js");
const { BankManagerControlBinding } = require("./BankManagerControlBinding.js");

class ControlBindings
{
    constructor()
    {
        this.items = {};
        this.presentationActive = true;
    }
    
    add(name, binding)
    {
        if (!name) throw new Error("Control binding requires a varname.");
        if (this.items.hasOwnProperty(name)) {
            throw new Error("Duplicate control binding varname: " + name);
        }
        if (binding) {
            binding.setPresentationActive(this.presentationActive);
            this.items[name] = binding;
        }
        return binding;
    }
    
    suspend()
    {
        Object.keys(this.items).forEach((name) => {
            this.items[name].suspend();
        }, this);
    }
    
    resumeLatest()
    {
        Object.keys(this.items).forEach((name) => {
            this.items[name].resumeLatest();
        }, this);
    }
    
    setPresentationActive(active)
    {
        this.presentationActive = Boolean(active);
        Object.keys(this.items).forEach((name) => {
            this.items[name].setPresentationActive(this.presentationActive);
        }, this);
    }
    
    handle(name, intent, values)
    {
        let binding = this.items[name];
        if (binding) {
            binding.handleIntent(intent, values || []);
        }
    }
    
    destroy()
    {
        Object.keys(this.items).forEach((name) => {
            this.items[name].destroy();
        }, this);
        this.items = {};
        this.presentationActive = false;
    }
}

module.exports = {
    ControlBindings: ControlBindings
};

