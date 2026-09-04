using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class DspTelemetryPublisher
{
    private readonly IPresentationTransport _transport;
    private readonly Func<InstanceId, ulong> _getRecipient;

    public DspTelemetryPublisher(
        IPresentationTransport transport,
        Func<InstanceId, ulong> getRecipient)
    {
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(getRecipient);
        _transport = transport;
        _getRecipient = getRecipient;
    }

    public void PublishInputLevel(
        InstanceId sourceInstanceId,
        uint revision,
        float levelDb,
        float fastLevelDb)
    {
        var recipient = _getRecipient(sourceInstanceId);
        if (recipient == 0)
        {
            return;
        }

        _transport.Send(new ProtocolOutput(
            [recipient],
            "dsp_telemetry",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
                new Atom(AtomType.Integer, revision, 0, null),
                new Atom(AtomType.Integer, 2, 0, null),
                new Atom(AtomType.Symbol, 0, 0, "input_gain.level"),
                new Atom(AtomType.Float, 0, levelDb, null),
                new Atom(AtomType.Symbol, 0, 0, "input_gain.level_fast"),
                new Atom(AtomType.Float, 0, fastLevelDb, null)
            ],
            DeliverySemantics.LatestAnalysis));
    }

    public void PublishTrackLevels(
        InstanceId recipient,
        uint revision,
        IReadOnlyList<TrackLevelTelemetry> levels)
    {
        ArgumentNullException.ThrowIfNull(levels);
        var atoms = new List<Atom>(3 + levels.Count * 2)
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, revision, 0, null),
            new(AtomType.Integer, levels.Count, 0, null)
        };
        foreach (var level in levels)
        {
            atoms.Add(new Atom(
                AtomType.Integer,
                (long)level.InstanceId.Value,
                0,
                null));
            atoms.Add(new Atom(AtomType.Float, 0, level.LevelDb, null));
        }

        _transport.Send(new ProtocolOutput(
            [recipient.Value],
            "track_levels",
            atoms,
            DeliverySemantics.LatestAnalysis));
    }
}
