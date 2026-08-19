using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Core.Abstractions;

public interface IDspStatePublisher
{
    void Publish(in DspSnapshot snapshot);

    void Stop();
}