using System;
using System.IO;
using System.Text;

using Consolidator.Managed.Core.Services.Persistence;
using Consolidator.Managed.Protocol;
using Consolidator.Managed.Tests.Support;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class PersistenceUseCasesTests
{
    [Fact]
    public void CaptureWaitsForEarlierCommandsAndRestoreCreatesAHistoryBaseline()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        var protocol = application.GetRequiredService<ProtocolService>();
        var persistence = application.GetRequiredService<InstancePersistenceService>();
        var writeRequest = application.Enqueue(
            instance,
            "write",
            Symbol("local"),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-18.0));

        var payload = protocol.ExecuteControlBarrier(() =>
            persistence.CaptureCommitted(instance.InstanceId));
        instance.Output.WaitForResponse(writeRequest);
        application.Send(
            instance,
            "write",
            Symbol("local"),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-12.0));
        instance.Output.Clear();

        protocol.ExecuteControlBarrier(() =>
        {
            persistence.Restore(instance.InstanceId, payload);
            return true;
        });

        Assert.Equal(-18.0F, instance.Dsp.Latest.CompressorThresholdDb);
        Assert.DoesNotContain(
            instance.Output.Messages,
            message => message.Selector == "persistence_dirty");

        application.Send(instance, "jump_history", Integer(0));
        Assert.Equal(-18.0F, instance.Dsp.Latest.CompressorThresholdDb);
        application.Send(instance, "jump_history", Integer(2));
        Assert.Equal(-18.0F, instance.Dsp.Latest.CompressorThresholdDb);
    }

    [Fact]
    public void UnsupportedSchemaIsRejectedWithoutChangingState()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        var persistence = application.GetRequiredService<InstancePersistenceService>();
        var payload = persistence.CaptureCommitted(instance.InstanceId);
        var invalidPayload = Encoding.UTF8.GetBytes(
            Encoding.UTF8.GetString(payload).Replace(
                "\"schema\":1",
                "\"schema\":2",
                StringComparison.Ordinal));
        var before = instance.Dsp.Latest.CompressorThresholdDb;
        var beforePublishCount = instance.Dsp.PublishCount;

        Assert.Throws<InvalidDataException>(() =>
            persistence.Restore(instance.InstanceId, invalidPayload));
        Assert.Equal(before, instance.Dsp.Latest.CompressorThresholdDb);
        Assert.Equal(beforePublishCount, instance.Dsp.PublishCount);
    }

    [Fact]
    public void RestorePublishesDerivedDspChangesForOtherInstances()
    {
        using var application = new ManagedApplicationFixture();
        var restoredInstance = application.RegisterInstance();
        var otherInstance = application.RegisterInstance();
        var persistence = application.GetRequiredService<InstancePersistenceService>();
        var payload = persistence.CaptureCommitted(restoredInstance.InstanceId);
        application.Send(
            restoredInstance,
            "set_instance_solo",
            Symbol("local"),
            Integer(1),
            Symbol("exclusive"));
        Assert.Equal(0U, otherInstance.Dsp.Latest.Audible);
        var publishCount = otherInstance.Dsp.PublishCount;

        persistence.Restore(restoredInstance.InstanceId, payload);

        Assert.Equal(1U, otherInstance.Dsp.Latest.Audible);
        Assert.True(otherInstance.Dsp.PublishCount > publishCount);
    }
}
