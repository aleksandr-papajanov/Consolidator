var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
var environment = require("../../support/ProductionEnvironment.js");

function loadHost(LiveAPI, outputs)
{
    var context = vm.createContext({
        LiveAPI: LiveAPI,
        Number: Number,
        String: String,
        isFinite: isFinite,
        outlet: function (index, message)
        {
            outputs.push([index, Array.prototype.slice.call(message)]);
        },
    });
    var sourcePath = path.join(
        environment.root,
        "js/LiveTrackIdentityHost.js");
    vm.runInContext(fs.readFileSync(sourcePath, "utf8"), context, {
        filename: "js/LiveTrackIdentityHost.js",
    });
    return context;
}

function testTrackResolutionPublishesCurrentNameAndObservesLaterChanges()
{
    var calls = [];
    var outputs = [];
    var observer = null;

    function FakeLiveAPI(callbackOrPath, livePath)
    {
        calls.push([callbackOrPath, livePath]);
        if (callbackOrPath === "this_device")
        {
            return {
                get: function (property)
                {
                    assert.strictEqual(property, "canonical_parent");
                    return ["id", 42];
                },
            };
        }
        if (callbackOrPath === "id 42")
        {
            return {
                get: function (property)
                {
                    assert.strictEqual(property, "name");
                    return ["Lead Vocal"];
                },
            };
        }

        assert.strictEqual(typeof callbackOrPath, "function");
        assert.strictEqual(livePath, "id 42");
        observer = { callback: callbackOrPath, property: "" };
        return observer;
    }

    var context = loadHost(FakeLiveAPI, outputs);
    context.bang();

    assert.deepStrictEqual(outputs, [[0, ["track_name", "Lead Vocal"]]]);
    assert.strictEqual(observer.property, "name");

    observer.callback(["name", "Backing Vocal"]);
    assert.deepStrictEqual(outputs[1], [0, ["track_name", "Backing Vocal"]]);

    context.freebang();
    assert.strictEqual(context.trackObserver, null);
    assert.strictEqual(calls.length, 3);
}

function testInvalidParentDoesNotPublishOrCreateAnObserver()
{
    var calls = 0;
    var outputs = [];
    function FakeLiveAPI(livePath)
    {
        calls += 1;
        assert.strictEqual(livePath, "this_device");
        return {
            get: function ()
            {
                return ["id", 0];
            },
        };
    }

    var context = loadHost(FakeLiveAPI, outputs);
    context.bang();

    assert.strictEqual(calls, 1);
    assert.deepStrictEqual(outputs, []);
    assert.strictEqual(context.trackObserver, null);
}

testTrackResolutionPublishesCurrentNameAndObservesLaterChanges();
testInvalidParentDoesNotPublishOrCreateAnObserver();
console.log("LiveTrackIdentityHostTests passed");
