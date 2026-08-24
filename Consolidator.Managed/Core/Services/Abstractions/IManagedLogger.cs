namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IManagedLogger
{
    void Info(string message);

    void Warning(string message);

    void Error(string message);
}




