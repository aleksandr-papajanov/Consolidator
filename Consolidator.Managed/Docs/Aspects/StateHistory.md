# Managed history and transactions

`StateHistory` owns one logical cursor and a deterministic registration-order
list of active `IHistoryValue` objects. The generic registry registers and
unregisters values directly, so history never discovers values by walking the
tree. The ordered collection keeps a direct node index, making registration
and removal O(1) without scanning values owned by previously loaded instances.

Presentation-only metadata and audibility controls (`Label`, `Mute`, `Solo`,
and `Bypass` values) remain normal state values for registry notifications but
are not registered as history values. They therefore change immediately
without creating or being restored by an Undo/Redo step. History is reserved
for editable DSP parameters and bank topology.

For a standalone ordinary `write` (`transactionId = 0`), Managed prepares the
entire operation, including dependent writes and peers, then advances history
only if the transaction actually changes a registered history value. No-op and
non-history edits create no history entry. Gesture writes carry their begin/end
transaction ID and use the gesture's existing history point; the final zero-ID
acknowledgement repeats the accepted values and creates no additional entry.
There is no frontend list of history-backed parameters.

`group_banks`, `ungroup_banks` and `clear_topology` own their history advancement
in Managed. Group creation validates every selected bank and allocates the group
ID under the operation gate before one atomic commit. Ungroup resolves members
from authoritative topology. Low-level `write topology` retains explicit history
bracketing for explicit topology write batches; it does not create a point per row.

`AdvanceHistoryPoint` copies the current slot into the next slot before a
logical edit. `JumpToHistory` moves the shared logical cursor in one operation,
then notifies observers for values whose effective value changed. Each successful
advance or jump increments the history revision and emits a snapshot containing
the revision, cursor, entry count and navigation availability. The protocol
publisher sends that snapshot to every currently registered instance.

`StateHistoryTransaction` prepares every write batch or recursive reset,
including every affected peer, before committing storage. Value observers run
only after all entries commit and pending state is cleared. If
preparation or storage commit fails, entries roll back in reverse order and no
observer event is emitted. Protocol delivery failures are caught by
`StateChangePublisher` and cannot turn a committed state operation into a
failed mutation.

History navigation refreshes the complete observed target. An analyzer target
transition cancels any local preview gesture, clears its transaction state and
discards control-side preview coordinates before the authoritative snapshot is
presented. A stale equalizer preview therefore cannot mask the restored curve or
prevent the next gesture from opening a new history point.

Each analyzer gesture owns exactly one history point. Its final write and
`end_history` frame are sent in FIFO order, while the originating gesture binding releases
the gesture transaction immediately after dispatch instead of waiting for the
write response. A delayed response therefore cannot merge later equalizer
gestures into the previous history point. The local curve preview remains until
that final write is acknowledged; an acknowledgement from an older gesture
cannot clear the preview of a newer gesture.

A persistence restore uses the same transaction mechanism but replaces every
slot of each persisted value with one validated baseline. Slots are replaced
even when the active slot already equals the restored value; observers still
receive only effective active-value changes. This prevents later history
navigation from exposing values that predate the loaded snapshot.

Root removal disposes its values while the shared operation gate is still held.
This unregisters them from `StateHistory` and observer registries before another
control operation can begin. Managed publisher and command-gate disposal may
then finish outside the shared gate without leaving stopped values visible.

## History protocol notifications

`begin_history` and `end_history` frame a global history action. `jump_history`
moves the shared `StateHistory` cursor to a logical history
position. Every successful history advance or jump publishes a `history_state`
snapshot to all registered instances; the snapshot contains revision, cursor,
entry count and navigation availability. UI initialization also publishes the
current snapshot so a newly connected editor can render existing history before
the next history mutation.
