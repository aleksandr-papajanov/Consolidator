using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed partial class StatePeerObserver
{
    private sealed partial class StatePeerValueObserver<TValue>
    {
        private static TValue Add(TValue left, TValue right) =>
            Calculate(left, right, (first, second) => first + second);

        private static TValue Subtract(TValue left, TValue right) =>
            Calculate(left, right, (first, second) => first - second);

        private static TValue Calculate(
            TValue left,
            TValue right,
            Func<float, float, float> operation)
        {
            if (typeof(TValue) != typeof(float))
            {
                throw new InvalidOperationException(
                    $"Delta editing is not supported for {typeof(TValue).Name}.");
            }

            return (TValue)(object)operation(
                (float)(object)left!,
                (float)(object)right!);
        }

        private static bool IsBankNode(NodeId node) =>
            node.Value >= 100 &&
            node.Value < 100 + DspConstants.BankCount;
    }
}