# VST3 JUCE theme

## Ownership and rendering

`Shared/Theme/UiLookAndFeel` provides one editor-local JUCE LookAndFeel for
TextButtons, ComboBox and its popup menu. `EditorPresentation` owns the theme,
declares it before all children so it outlives them, and detaches it in its
destructor. The application-wide default LookAndFeel is never changed.

`UiMetrics` defines Arial 12 for ordinary controls and labels, 28-unit button
and ComboBox height and 4-unit spacing. Buttons and ComboBox have square corners;
there is no corner-radius setting. Processor headings
use 16 and the window title 24. Width follows the label or table-cell role;
S/M/R/B retain compact single-letter slots. The fixed canvas scales these
metrics uniformly at 75/100/125%.

All standalone actions, parameter toggles and Bank Manager S/M/R/B controls
are JUCE TextButtons. No control subclass
implements a separate button painter. Bank selection cells, processor markers,
curves, dials and Character indicator remain purpose-specific drawing.

## Behavior

`ActionButton` extends TextButton only with the existing 180 ms action flash,
published through a theme property; its reusable timer stops on destruction.
`ParameterButton` extends TextButton only with mouse-down triggering and the
double-click reset flag. Accepted parameter/row state drives toggle presentation;
JUCE does not toggle committed values locally when clicked.

Accepted-value controls derive their on/off fill and text only from accepted
toggle state, without a transient on color from mouse-down or hover brightness.
This removes the pressed-on/old-value/confirmed-on color sequence while a
Managed write is pending. Explicit ActionButton flashes remain unchanged.

Theme properties distinguish transient action flashing and accepted-value
presentation. Input locking retains the accepted value colors instead of
dimming every parameter. Other unavailable actions display disabled text.
Bank scope and mute/reset colors remain semantic per-control overrides of the
same shared renderer.

Group/Local is a navigation control: it is enabled only when the focused bank
belongs to a group and navigation is available. It does not opt into preserved
accepted-value disabled text, so Local visibly dims when no group choice exists.
Parameter toggle group dots are supplied as an ARGB theme property and drawn
inside the button's top-right corner by the shared JUCE renderer. The toggle
wrapper has the same 88 x 28 bounds as its button, centered in the original
fixed channel-strip slot; dials and choice controls keep their existing bounds.

Bank Manager rows have a fixed 24-unit height. Bank cells and S/M/R/B buttons are
24 x 24 squares. Processor markers are 8 x 8, centered in 12 x 24 hit slots;
active outlines and effect/bypass dots retain the accepted registry indicators.
Each row keeps its original horizontal order.
Track output-level dots are 9 units in diameter, centered vertically; bank
activity/bypass badges are 8-unit squares with a one-pixel contrasting border.
The table width grows with the wider processor slots, preserving track-label
space, and the label inset includes the enlarged level dot.
The first row starts at the table's top edge, with horizontal padding only.
Bank-grid vertical lines stop at the final row's closing border rather than
extending through the empty viewport. Scroll bounds include rows and that
one-pixel closing border; hit testing uses the same integer scroll offset as
row painting and child-button positioning. An empty table has no grid lines.
The first bank row also has a one-pixel top border drawn after cell fills;
the table grid owns this border independently of JUCE S/M/R/B outlines.
S/M/R/B use JUCE connected-edge flags: the shared theme omits the right/bottom
border when an adjacent button owns that boundary through its left/top border.
Shared borders are one logical pixel wide, including between rows; standalone
buttons retain a complete one-pixel outline. Child buttons
are created/deleted on registry topology updates, reused for presentation
updates, repositioned when scrolling and hidden outside the viewport. Latest
accepted row state is read when clicked; normal/Shift Solo and reset targeting
retain the existing contracts. The old row-reset feedback map and table timer
were removed; Reset uses the shared ActionButton behavior.

`InstanceButtonsControl` owns the four JUCE S/M/R/B buttons and their common
presentation/click behavior. The table owns one strip per registry row; the
processing header reuses the same class for the currently observed instance.
Only table strips connect vertically across rows. Both locations share the
same accepted registry/model state and existing host command dispatcher; no
additional observer or independent application state is created.

Analyzer filters have no separate buttons. Graph handles use Ctrl-click for
bypass and double-click for reset; bypassed handles draw as dim outlines.

## Validation

Current host scenarios and diagnostic procedure: [Validation](Validation.md).
Build/install success does not establish visual or interaction correctness.
