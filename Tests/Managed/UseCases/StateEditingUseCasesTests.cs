using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Tests.Support;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class StateEditingUseCasesTests
{
    [Fact]
    public void WriteReadAndResetFlowUpdatesStateNotificationsAndDspProjection()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();
        var initialPublishCount = instance.Dsp.PublishCount;

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("input_gain"),
            Symbol("gain"),
            Symbol("value"),
            Float(6.0));

        Assert.Equal(6.0F, instance.Dsp.Latest.Gain);
        Assert.True(instance.Dsp.PublishCount > initialPublishCount);
        Assert.Equal(1, instance.Output.Single("action_done").Atoms[^1].Integer);
        var change = instance.Output.Single("state_changed");
        Assert.Equal("input_gain.gain", change.Atoms[1].Symbol);
        Assert.Equal(6.0, change.Atoms[2].Float);

        instance.Output.Clear();
        application.Send(
            instance,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("input_gain"),
            Symbol("gain"));

        Assert.Equal(6.0, instance.Output.Single("state_done").Atoms[^1].Float);

        instance.Output.Clear();
        application.Send(
            instance,
            "reset",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Symbol("input_gain"));

        Assert.Equal(1.0F, instance.Dsp.Latest.Gain);
        Assert.Equal(1, instance.Output.Single("action_done").Atoms[^1].Integer);
        Assert.Contains(
            instance.Output.Messages,
            message => message.Selector == "state_changed" &&
                message.Atoms[1].Symbol == "input_gain.gain" &&
                message.Atoms[2].Float == 1.0);
    }

    [Fact]
    public void MalformedWriteReturnsProtocolErrorWithoutChangingDspState()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();
        var initial = instance.Dsp.Latest.Gain;
        var publishCount = instance.Dsp.PublishCount;

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("input_gain"),
            Symbol("gain"));

        Assert.Equal(initial, instance.Dsp.Latest.Gain);
        Assert.Equal(publishCount, instance.Dsp.PublishCount);
        Assert.Equal("error", Assert.Single(instance.Output.Messages).Selector);
    }
}
