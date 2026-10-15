# Track and bank interaction

## Track table

Tracks BankManagerControl composes the table and topology action toolbar.
Tracks/Navigation supplies the reusable bank/group/member controls embedded by
Equalizer. Both borrow TracksViewModel and emit the same TrackIntent types.

One full registry request initializes a staged presentation snapshot. It is
published only after its matching `registry_done`, complete instance/bank/group
counts and consistent received membership frames. Revisioned deltas update the current
cache directly. A revision gap requests one replacement snapshot; an older
replacement cannot overwrite a newer revision. Processor-marker batches and
latest track-level telemetry do not advance topology revision or refetch rows.

The table presents six passive processor markers, seven bank cells,
group letters/colors, activity dots and instance `S/M/R/B` controls. The output
level dot uses the existing nonlinear `-60..0 dBFS` mapping. Scrolling and
selection remain local to the control. Shift-click/Shift-drag toggles ungrouped
banks, visits each crossed cell once and cannot select a second bank from the
same instance. Selecting a new grouped bank selects group scope; selecting the
already focused bank preserves a manually chosen local scope.
Every ordinary bank click clears previous explicit selections first, including
clicks on grouped or already focused banks. Only an ungrouped bank is then
explicitly selected. Shift selection does not change focus or clear unrelated
selections. Processor markers do not select targets or send commands.

Group and Ungroup send one intent each. Managed allocates the first unused
user group `1..15`, validates every member, and commits membership changes
atomically with one history point. Ungroup resolves the complete membership
from Managed topology; automatic group zero is immutable. The frontend has no
row-write loop, group-ID allocation or topology history wrapper.
Clear uses the existing coordinator command, with an asynchronous confirmation,
and preserves automatic group A.

Bank Ctrl-click toggles bypass; Ctrl-double-click requests targeted reset.
Device headings use shared MarkerControl: click toggles bypass, double-click
resets the device without Ctrl. The table markers visualize ActivityObserver's
committed effectActive and bypass using the same rendering as device headings:
active fills the square, neutral leaves an outline, and bypass makes either gray.
Solo is exclusive on normal click and additive with Shift; mute and
instance bypass are additive. Their scope follows the original focused-instance
rule. Instance Reset retains the existing current-target `dsp` reset behavior.
EqualizerController sends Commit and Refine through the existing Managed
operations, including their own history advancement after successful approximation.

Gestures send begin/end intents. Standalone parameter writes and topology,
Reset, Commit and Refine operations create their history points in Managed;
there is no frontend per-parameter history policy. Toggle
double-click sends reset; Managed performs any required Leveler freeze in the
same transaction and preserves each target's own slow gain.
Instance mute/solo/bypass and table bypass controls remain direct commands.

