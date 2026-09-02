using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class SpectrumPublisher
{
    private const int FftSize = 1024;

    private readonly IPresentationTransport _transport;
    private readonly Func<InstanceId, ulong> _getRecipient;

    public SpectrumPublisher(
        IPresentationTransport transport,
        Func<InstanceId, ulong> getRecipient)
    {
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(getRecipient);
        _transport = transport;
        _getRecipient = getRecipient;
    }

    public void Publish(
        InstanceId sourceInstanceId,
        float[] mainSpectrum,
        float[] referenceSpectrum)
    {
        var targetId = _getRecipient(sourceInstanceId);
        if (targetId == 0)
        {
            return;
        }

        var atoms = new List<Atom>(3 + mainSpectrum.Length + referenceSpectrum.Length)
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
            new(AtomType.Integer, FftSize, 0, null)
        };
        foreach (var value in mainSpectrum)
        {
            atoms.Add(new Atom(AtomType.Float, 0, value, null));
        }
        foreach (var value in referenceSpectrum)
        {
            atoms.Add(new Atom(AtomType.Float, 0, value, null));
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