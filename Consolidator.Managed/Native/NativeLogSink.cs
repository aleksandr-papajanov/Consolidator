using System.Text;

namespace Consolidator.Managed.Native;

public unsafe sealed class NativeLogSink
{
    public static NativeLogSink Shared { get; } = new();

    private delegate* unmanaged[Cdecl]<void*, byte*, void> _callback;
    private void* _context;

    private NativeLogSink()
    {
    }

    public void Configure(
        void* context,
        delegate* unmanaged[Cdecl]<void*, byte*, void> callback)
    {
        _context = context;
        _callback = callback;
    }

    public void Write(string message)
    {
        if (_callback == null)
        {
            return;
        }

        var bytes = Encoding.UTF8.GetBytes(message + "\0");
        fixed (byte* text = bytes)
        {
            _callback(_context, text);
        }
    }
}




