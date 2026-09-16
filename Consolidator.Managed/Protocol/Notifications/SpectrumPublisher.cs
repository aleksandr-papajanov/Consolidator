using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class SpectrumPublisher
{
    private const int FftSize = 1024;

    private readonly IPresentationTransport _transport;

    public SpectrumPublisher(IPresentationTransport transport)
    {
        ArgumentNullException.ThrowIfNull(transport);
        _transport = transport;
    }

    public void Publish(
        ulong targetId,
        InstanceId sourceInstanceId,
        float[] sourceSpectrum,
        float[]? groupSpectrum)
    {
        if (targetId == 0)
        {
            return;
        }

        var atomCount = 4 + sourceSpectrum.Length +
            (groupSpectrum?.Length ?? 0);
        var atoms = new List<Atom>(atomCount)
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
            new(AtomType.Integer, FftSize, 0, null),
            new(AtomType.Integer, groupSpectrum is null ? 0 : 1, 0, null)
        };
        foreach (var value in sourceSpectrum)
        {
            atoms.Add(new Atom(AtomType.Float, 0, value, null));
        }

        if (groupSpectrum is not null)
        {
            foreach (var value in groupSpectrum)
            {
                atoms.Add(new Atom(AtomType.Float, 0, value, null));
            }
        }

        _transport.Send(new ProtocolOutput(
            [targetId],
            "fft",
            atoms,
            DeliverySemantics.LatestAnalysis));
        RuntimeMetrics.Shared.ForInstance(sourceInstanceId.Value)
            .RecordFftFrame();
    }
}
