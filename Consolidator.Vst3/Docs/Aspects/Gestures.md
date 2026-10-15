# User gestures and acknowledgements

## Continuous gestures

Pointer down opens one Managed history point. Moves received before its
acknowledgement retain only the latest pending value; release before the
acknowledgement retains the final position and pending end. Once ready, a
reused 33 ms JUCE timer submits the latest dirty value through the existing
coalescing transaction contract. Successful callbackless intermediate writes
do not create pending response entries.

Release submits the final transaction write, then an acknowledged transaction
zero write, then closes history. Notifications precede the final reply in the
lossless FIFO. Parameter controls retain their optimistic preview until that
reply; acknowledged updates change their authoritative presentation cache
without moving the active drag backward. Rejection clears preview and requests
the current target snapshot. Closing a window queues its final position and
history end before detaching its listener; no reply retains a destroyed editor.

Bank, scope and topology navigation are disabled during a gesture.
Only its active control remains enabled, and it is disabled while awaiting
the final acknowledgement. Group float edits use `delta`, preserving peer
offsets, and clamp to Managed's effective range. Local floats and discrete
values use `copy`. Effective-range notifications update the same parameter
presentation; there are no frontend DSP defaults.

EQ snapshot paths use `equalizer.filter.N...`, while writes use
`equalizer.bank.filter.N...` to select the observed bank. This is the current relative-path encoding contract.

