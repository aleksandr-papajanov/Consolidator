# Max UI performance

Target snapshot application is a synchronous presentation batch. State values
may notify their subscribers while the batch is open, but presenters defer
rebuilds and each affected presenter rebuilds once when the batch closes.
Bindings remain suspended until the target transition completes, so the latest
presentation is sent once for each active control.

Inactive UI instances do not request an initial target snapshot. The first
activation requests the current target after Managed acknowledges activation.

`PanelBindingHostV8` caches named patcher controls because panel object identity is
stable for the lifetime of the panel. Button presentation updates are framed by
`presentation_begin` and `presentation_end`, allowing the control to redraw once
for the complete update. Analyzer redraw and drag scheduling reuse one task per
purpose rather than allocating a task for each update.

`PanelBindingHostV8` is the first modern-engine pilot. It uses an ES6 class,
`Map`, rest/spread arguments and destructuring, and is instantiated by `v8`
objects in all panels.
