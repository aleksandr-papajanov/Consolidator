autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

let taskCompleted = false;
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

const task = new Task(() =>
{
    taskCompleted = true;
    mgraphics.redraw();
    outlet(0, ["max9_ui_test", "passed", "Task callback and repaint"]);
    post("max9_ui_test passed: Task callback and repaint\n");
    appendResult("max9_ui_test passed: Task callback and repaint");
});

function paint()
{
    mgraphics.set_source_rgba(0.1, 0.1, 0.1, 1);
    mgraphics.rectangle(0, 0, mgraphics.size[0], mgraphics.size[1]);
    mgraphics.fill();
}

function onresize()
{
    mgraphics.redraw();
}

function run()
{
    resultPath = String(this.patcher.filepath).replace(
        /[^\\/]+$/,
        "Max9HostTestResult.txt");
    if (typeof mgraphics.redraw !== "function")
    {
        outlet(0, ["max9_ui_test", "failed", "mgraphics unavailable"]);
        return;
    }

    taskCompleted = false;
    task.schedule(1);
    mgraphics.redraw();
    outlet(0, ["max9_ui_test", "scheduled", taskCompleted ? 1 : 0]);
    post("max9_ui_test scheduled\n");
    appendResult("max9_ui_test scheduled");
}

function bang()
{
    run();
}

function notifydeleted()
{
    task.cancel();
}
