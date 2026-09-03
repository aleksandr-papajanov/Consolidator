const {
    decodeOptionalStateValue,
    decodeStateValue
} = require("./StateWireCodec.js");

class StateResponseAssembler
{
    constructor()
    {
        this.responses = {};
    }

    begin(args)
    {
        if (args.length !== 6)
        {
            return;
        }

        this.responses[String(args[2])] = {
            instanceId: String(args[3]),
            truncated: Number(args[4]) === 1,
            expectedCount: Number(args[5]),
            entries: [],
            invalid: false
        };
    }

    addEntry(args)
    {
        const response = this.responses[String(args[2])];
        if (!response || args.length < 12)
        {
            return;
        }
        if (Number(args[4]) !== response.entries.length)
        {
            response.invalid = true;
            return;
        }

        const path = args.slice(5, -6).join(".");
        response.entries.push({
            path: path,
            value: decodeStateValue(args[args.length - 6]),
            status: decodeOptionalStateValue(args[args.length - 5]),
            physicalMin: decodeOptionalStateValue(args[args.length - 4]),
            physicalMax: decodeOptionalStateValue(args[args.length - 3]),
            min: decodeOptionalStateValue(args[args.length - 2]),
            max: decodeOptionalStateValue(args[args.length - 1]),
            instanceId: String(args[3])
        });
    }

    finish(args)
    {
        if (args.length !== 4)
        {
            return null;
        }

        const requestId = String(args[2]);
        const response = this.responses[requestId];
        delete this.responses[requestId];
        if (!response || String(args[3]) !== response.instanceId ||
                response.invalid || response.entries.length !== response.expectedCount)
        {
            return {
                requestId: requestId,
                result: { entries: [], error: "malformed_state_response" }
            };
        }
        return {
            requestId: requestId,
            result: {
                entries: response.entries,
                truncated: response.truncated,
                error: response.truncated ? "state_response_truncated" : null
            }
        };
    }

    clear()
    {
        this.responses = {};
    }
}

module.exports = {
    StateResponseAssembler: StateResponseAssembler
};
