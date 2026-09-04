namespace Consolidator.Managed.Protocol.Notifications;

internal readonly record struct TrackLevelTelemetry(
    InstanceId InstanceId,
    float LevelDb);
