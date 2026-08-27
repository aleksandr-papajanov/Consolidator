inlets = 1;
outlets = 1;

const requiredNames = ["panel_router", "ui_probe"];
let resultPath = "";

function appendResult(line)
{
    if (!resultPath)
    {
        return;
    }
    const file = new File(resultPath, "readwrite");
    if (!file.isopen)
    {
        return;
    }
    file.position = file.eof;
    file.writeline(line);
    file.close();
}

function resetResult(patcher)
{
    resultPath = String(patcher.filepath).replace(
        /[^\\/]+$/,
        "Max9HostTestResult.txt");
    const file = new File(resultPath, "write");
    if (file.isopen)
    {
        file.close();
    }
}

function report(status, detail)
{
    outlet(0, ["max9_host_test", status, detail]);
    post("max9_host_test " + status + ": " + detail + "\n");
    appendResult("max9_host_test " + status + ": " + detail);
}

function assertNamedObjects(patcher)
{
    if (!patcher || typeof patcher.getnamed !== "function")
    {
        report("failed", "patcher.getnamed unavailable");
        return false;
    }

    for (const name of requiredNames)
    {
        const object = patcher.getnamed(name);
        if (!object)
        {
            report("failed", "missing " + name);
            return false;
        }
    }
    return true;
}

function run(patcher)
{
    resetResult(patcher);
    if (!assertNamedObjects(patcher))
    {
        return;
    }

    report("passed", "named v8 router and v8ui probe resolved");

    const router = patcher.getnamed("panel_router");
    if (router)
    {
        router.message("msg_list", 0, "ui_probe", "run");
    }
}

function loadbang()
{
    run(this.patcher);
}

function bang()
{
    run(this.patcher);
}
