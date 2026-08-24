using Consolidator.Managed.Tests.Support;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class HistoryUseCasesTests
{
    [Fact]
    public void FramedEditAndHistoryJumpRestoreStateForEveryRegisteredClient()
    {
        using var application = new ManagedApplicationFixture();
        var editor = application.RegisterInstance();
        var observer = application.RegisterInstance();
        editor.Output.Clear();
        observer.Output.Clear();

        application.Send(editor, "begin_history", Symbol("41"));
        application.Send(
            editor,
            "write",
            Symbol(editor.InstanceId.Value.ToString()),
            Symbol("41"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-18.0));
        application.Send(editor, "end_history", Symbol("41"));

        Assert.Equal(-18.0F, editor.Dsp.Latest.CompressorThresholdDb);
        Assert.Contains(
            observer.Output.Messages,
            message => message.Selector == "history_state" &&
                message.Atoms[3].Integer > 0 &&
                message.Atoms[4].Integer == 1);

        editor.Output.Clear();
        observer.Output.Clear();
        application.Send(editor, "jump_history", Integer(0));

        Assert.NotEqual(-18.0F, editor.Dsp.Latest.CompressorThresholdDb);
        Assert.Contains(
            editor.Output.Messages,
            message => message.Selector == "state_changed" &&
                message.Atoms[1].Symbol == "compressor.threshold");
        Assert.Contains(
            observer.Output.Messages,
            message => message.Selector == "history_state" &&
                message.Atoms[2].Integer == 0 &&
                message.Atoms[4].Integer == 0 &&
                message.Atoms[5].Integer == 1);
    }
}
