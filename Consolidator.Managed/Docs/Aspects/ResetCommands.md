# State reset commands

Reset writes the target
state subtree's initial values through one prepared transaction, so peer
propagation remains authoritative and observers see only the complete reset.
The reset frame carries an explicit `local`, `group`, or `group_instance` scope. Local reset
prepares only the addressed state values. Group reset prepares every resolved
peer with that peer's own initial value; it never applies a delta from the
source value. `group_instance` resolves the exact group of the selected bank and
resets the complete DSP tree of each member instance, including every equalizer
bank.
Its body is:

```text
reset transactionId local|group|group_instance statePath...
```

Targeted reset uses the same `reset` selector and addresses the target before
the transaction fields:

```text
reset target targetInstanceId bankIndex|none transactionId local|group|group_instance statePath...
```

`bankIndex` is required for an equalizer bank reset and is `none` for a
processor reset. Targeted reset never changes selection and can address an
instance other than the source external. The Bank Manager uses targeted reset
only for Ctrl/Command-double-click; an ordinary double-click does not reset.

Processor-panel `R` uses `local` for a regular click and `group` for a
Ctrl/Command-click. Managed resolves the focused-bank context for the source
external and uses it during propagation.
The reset selector uses the same relative path grammar as state writes. A leaf
resets one value; `equalizer.filter.N` resets a complete filter;
`equalizer.bank` resets the focused equalizer bank; a processor path such as
`shaper` and `compression` reset their corresponding processor, and `dsp`
resets all DSP settings.
The VST3 UI emits one `reset` intent for a double-click and never supplies the
default value itself.
Resetting an already neutral subtree succeeds without creating a history point.
Resets that affect only non-history values also leave the history cursor unchanged.
