var assert = require("assert");
var fs = require("fs");
var path = require("path");
var environment = require("../../support/ProductionEnvironment.js");

function walkFiles(directory, extension)
{
    var files = [];
    fs.readdirSync(directory, { withFileTypes: true }).forEach(function (entry)
    {
        var entryPath = path.join(directory, entry.name);
        if (entry.isDirectory())
        {
            files = files.concat(walkFiles(entryPath, extension));
        }
        else if (path.extname(entry.name) === extension)
        {
            files.push(entryPath);
        }
    });
    return files;
}

function resolveProjectPath(projectPath)
{
    var relativePath = projectPath.substring("Project:/".length);
    var direct = path.join(environment.root, relativePath);
    if (fs.existsSync(direct))
    {
        return direct;
    }

    var basename = path.basename(relativePath);
    var matches = walkFiles(environment.root, path.extname(basename)).filter(
        function (candidate)
        {
            return path.basename(candidate) === basename;
        });
    return matches.length === 1 ? matches[0] : null;
}

function assertProjectPath(projectPath, sourceFile)
{
    assert.ok(
        projectPath.indexOf("Project:/") === 0,
        sourceFile + " uses a context-dependent runtime path: " + projectPath);
    assert.ok(
        resolveProjectPath(projectPath),
        sourceFile + " references a missing project resource: " + projectPath);
}

function visitPatcherValue(value, sourceFile)
{
    if (!value || typeof value !== "object")
    {
        return;
    }

    if (value.maxclass === "jsui" || value.maxclass === "v8ui")
    {
        assertProjectPath(value.filename, sourceFile);
        if (value.maxclass === "v8ui")
        {
            assert.strictEqual(
                value.text,
                undefined,
                sourceFile + " passes an unsupported positional argument to v8ui");
        }
    }
    if (value.maxclass === "newobj" && /^(js|v8)\s/.test(value.text || ""))
    {
        var scriptPath = value.text.split(/\s+/)[1];
        assertProjectPath(scriptPath, sourceFile);
        var metadataFilename = value.textfile
            ? value.textfile.filename
            : value.saved_object_attributes.filename;
        assert.strictEqual(metadataFilename, scriptPath);
    }
    if (value.maxclass === "bpatcher")
    {
        var panelPath = [
            path.join(environment.root, "Panels", value.name),
            path.join(environment.root, "patchers", value.name),
            path.join(environment.root, value.name)
        ];
        assert.ok(
            value.name.indexOf("Project:/") === 0
                ? resolveProjectPath(value.name)
                : panelPath.some(function (candidate) {
                    return fs.existsSync(candidate);
                }),
            sourceFile + " references a missing panel resource: " + value.name);
    }

    Object.keys(value).forEach(function (key)
    {
        visitPatcherValue(value[key], sourceFile);
    });
}

function testEveryMaxRuntimeDependencyResolvesFromThePackage()
{
    var patchers = walkFiles(
        path.join(environment.root, "patchers"),
        ".maxpat").concat(walkFiles(
            path.join(environment.root, "Panels"),
            ".maxpat"));

    patchers.forEach(function (patcherPath)
    {
        var document = JSON.parse(fs.readFileSync(patcherPath, "utf8"));
        visitPatcherValue(
            document,
            path.relative(environment.root, patcherPath));
    });

    walkFiles(path.join(environment.root, "js"), ".js").forEach(function (file)
    {
        var source = fs.readFileSync(file, "utf8");
        var expression = /include\("([^"]+)"\)/g;
        var match = null;
        while ((match = expression.exec(source)) !== null)
        {
            assertProjectPath(match[1], path.relative(environment.root, file));
        }
    });
}

function testDevicePackageHasOneNativeExternalAndOneManagedBridge()
{
    var patcherText = walkFiles(environment.root, ".maxpat").map(function (file)
    {
        return fs.readFileSync(file, "utf8");
    }).join("\n");
    var externalCount = (patcherText.match(/"text"\s*:\s*"ConsolidatorExternal"/g) || []).length;
    var bridgeCount = (patcherText.match(/ConsolidatorUiHost\.js/g) || []).length;

    assert.strictEqual(externalCount, 1, "Max package must own exactly one native external");
    assert.ok(bridgeCount >= 1, "Managed protocol bridge is missing from the Max package");
    assert.ok(
        fs.existsSync(path.join(environment.root, "Consolidator.amxd")),
        "Ableton device entrypoint is missing");
}

testEveryMaxRuntimeDependencyResolvesFromThePackage();
testDevicePackageHasOneNativeExternalAndOneManagedBridge();
console.log("RuntimePathTests passed");
