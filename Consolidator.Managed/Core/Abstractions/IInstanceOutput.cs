using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core.Abstractions;

public interface IInstanceOutput
{
    void Send(
        string selector,
        ReadOnlySpan<Atom> atoms);
}