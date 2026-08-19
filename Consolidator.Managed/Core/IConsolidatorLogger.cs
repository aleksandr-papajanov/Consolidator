namespace Consolidator.Managed.Core;

public interface IConsolidatorLogger
{
    void Info(string message);

    void Warning(string message);

    void Error(string message);
}
