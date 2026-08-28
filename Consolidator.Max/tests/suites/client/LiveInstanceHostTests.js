var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
var environment = require("../../support/ProductionEnvironment.js");

function loadHost(LiveAPI, outputs)
{
    var context = vm.createContext({
        LiveAPI: LiveAPI,
        Array: Array,
        Number: Number,
        String: String,
        isFinite: isFinite,
        outlet: function (index, message)
        {
            outputs.push([index, Array.prototype.slice.call(message)]);
        },
    });
    var sourcePath = path.join(environment.root, "js/LiveInstanceHost.js");
    vm.runInContext(fs.readFileSync(sourcePath, "utf8"), context, {
        filename: "js/LiveInstanceHost.js",
    });
    return vm.runInContext(
        "new LiveInstanceHost(LiveAPI, function (message) { " +
        "outlet(0, message); })",
        context,
    );
}

function testSelectedDevicePublishesTrackNameAndExclusiveActivity()
{
    var outputs = [];
    var observers = {};

    function FakeLiveAPI(callbackOrPath, livePath)
    {
        if (callbackOrPath === null && livePath === "this_device")
        {
            return {
                id: 99,
            };
        }
        if (callbackOrPath === null &&
                livePath === "this_device canonical_parent")
        {
            return {
                id: 42,
                unquotedpath: "live_set tracks 3",
                get: function (property)
                {
                    assert.strictEqual(property, "name");
                    return ["Lead Vocal"];
                },
            };
        }

        assert.strictEqual(typeof callbackOrPath, "function");
        var observer = {
            callback: callbackOrPath,
            property: "",
            get: function (property)
            {
                if (property === "selected_track") return ["id", 42];
                throw new Error("Unexpected property " + property);
            },
        };
        observers[livePath] = observer;
        return observer;
    }

    var context = loadHost(FakeLiveAPI, outputs);
    context.bang();

    assert.deepStrictEqual(outputs, [
        [0, ["track_name", "Lead Vocal"]],
        [0, ["instance_active", 1]],
    ]);
    assert.strictEqual(observers["id 42"].property, "name");
    assert.strictEqual(observers["live_set view"].property, "selected_track");
    observers["live_set view"].callback(["selected_track", "id", 43]);
    assert.deepStrictEqual(outputs[2], [0, ["instance_active", 0]]);
    observers["live_set view"].callback(["selected_track", "id", 42]);
    assert.deepStrictEqual(outputs[3], [0, ["instance_active", 1]]);
    observers["id 42"].callback(["name", "Backing Vocal"]);
    assert.deepStrictEqual(outputs[4], [0, ["track_name", "Backing Vocal"]]);

    context.destroy();
    assert.strictEqual(context.trackObserver, null);
}

function testInvalidParentDoesNotPublishOrCreateObservers()
{
    var outputs = [];
    function FakeLiveAPI(callback, livePath)
    {
        assert.strictEqual(callback, null);
        if (livePath === "this_device")
        {
            return { id: 99 };
        }
        assert.strictEqual(livePath, "this_device canonical_parent");
        return { id: 0 };
    }

    var context = loadHost(FakeLiveAPI, outputs);
    context.bang();

    assert.deepStrictEqual(outputs, []);
    assert.strictEqual(context.trackObserver, null);
    assert.strictEqual(context.selectedTrackObserver, null);
}

testSelectedDevicePublishesTrackNameAndExclusiveActivity();
testInvalidParentDoesNotPublishOrCreateObservers();
console.log("LiveInstanceHostTests passed");
