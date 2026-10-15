using Consolidator.Managed.Native;
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

    public void PublishRuntime(InstanceId sourceInstanceId, DspTelemetrySnapshot snapshot)
    {
        var recipient = _getRecipient(sourceInstanceId);
        if (recipient == 0)
        {
            return;
        }
        var mask = (DspTelemetryValues)snapshot.ValuesPresent;
        var values = new List<(string Path, float Value)>();
        if ((mask & DspTelemetryValues.InputLevel) != 0)
        {
            values.Add(("gain_staging.input_level", snapshot.InputLevelDb));
            values.Add(("gain_staging.input_level_fast", snapshot.InputLevelFastDb));
        }
        else if ((mask & DspTelemetryValues.InputRms) != 0)
        {
            values.Add(("gain_staging.input_target", snapshot.InputRmsDb));
        }
        if ((mask & DspTelemetryValues.OutputGain) != 0)
        {
            values.Add(("gain_staging.output_level", snapshot.OutputGainDb));
            values.Add(("gain_staging.output_level_fast", snapshot.OutputGainFastDb));
        }
        else if ((mask & DspTelemetryValues.OutputRms) != 0)
        {
            values.Add(("gain_staging.output_target", snapshot.OutputRmsDb));
        }
        if ((mask & DspTelemetryValues.CompressorReduction) != 0)
        {
            values.Add(("compression.reduction_db", snapshot.CompressorReductionDb));
        }
        if ((mask & DspTelemetryValues.LimiterReduction) != 0)
        {
            values.Add(("gain_staging.limiter_reduction_db", snapshot.LimiterReductionDb));
        }
        var atoms = new List<Atom>(4 + values.Count * 2)
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
            new(AtomType.Integer, snapshot.Sequence, 0, null),
            new(AtomType.Integer, values.Count, 0, null)
        };
        foreach (var (path, value) in values)
        {
            atoms.Add(new Atom(AtomType.Symbol, 0, 0, path));
            atoms.Add(new Atom(AtomType.Float, 0, value, null));
        }
        _transport.Send(new ProtocolOutput([recipient], "dsp_telemetry", atoms,
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
