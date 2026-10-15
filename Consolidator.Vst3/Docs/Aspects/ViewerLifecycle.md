# Viewer activation and target transitions

## Viewer lifecycle

Managed still has one active viewer. The JUCE frontend mirrors that contract
on its message thread: activating another window disables the previous
window's editing, submits its final gesture writes and clears preview. Foreground
focus selects an existing window again. There is no Activate action and no independent Managed viewer.
Activation invalidates a pending registry assembly, obtains current registry
and target snapshots and resumes editing after the matching target snapshot.
Deleting the observed instance cancels its gesture and returns focus to the
editor's own instance.

Windows editor focus now selects the active viewer automatically. A 30 Hz
message-thread timer compares the editor peer's root HWND with the foreground
window, including focus on the host's floating-window title bar. Moving between
controls does not reconnect the viewer. The last viewer remains active while
focus is outside plugin editors, including transient menus. Focusing another
editor finishes the previous gesture before queuing activation. Native ownership
is claimed before sending the request; replies must match both that owner and
its activation generation. This prevents delayed acknowledgements from taking
ownership back. There is no Activate button. After an activation or presentation
error, leaving and refocusing the editor requests reconnection.

An unfocused or inactive editor displays a full-window black overlay at 65%
opacity with centered `Inactive` text. It intercepts pointer and keyboard
input; the first click focuses the window without editing a covered control.
The overlay stays visible until the window is focused and Managed activation
has been acknowledged. Ordinary busy operations in the active window use the
existing disabled controls. The overlay is presentation only and is not persisted.
Host focus/input scenarios: [Validation](Validation.md).

