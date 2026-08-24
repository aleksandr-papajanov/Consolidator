namespace Consolidator.Managed.Core.Dsp;

public interface IDspStatePublisher
{
    void Publish(in DspSnapshot snapshot);

    void Stop();
}



