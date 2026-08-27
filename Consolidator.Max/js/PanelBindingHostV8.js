inlets = 1;
outlets = 0;

class PanelBindingRouter
{
    constructor()
    {
        this.controls = new Map();
    }

    route(patcher, controlId, args)
    {
        if (args.length === 0 || !patcher ||
                typeof patcher.getnamed !== "function")
        {
            return;
        }
        let control = this.controls.get(controlId);
        if (!control)
        {
            control = patcher.getnamed(controlId);
            if (control)
            {
                this.controls.set(controlId, control);
            }
        }
        if (!control || typeof control.message !== "function")
        {
            return;
        }
        control.message(...args);
    }
}

const router = new PanelBindingRouter();

function handleListMessage(patcher, args)
{
    if (args.length < 3 || Number(args[0]) !== 0)
    {
        return;
    }
    const [, controlId, ...messageArgs] = args;
    router.route(patcher, String(controlId), messageArgs);
}

function list(...args)
{
    handleListMessage(this.patcher, args);
}

function msg_list(...args)
{
    handleListMessage(this.patcher, args);
}

function anything(...args)
{
    handleListMessage(this.patcher, args);
}
