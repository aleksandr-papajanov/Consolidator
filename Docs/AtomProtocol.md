# Consolidator Atom Protocol

Runtime communication uses Max atom lists. Dictionaries are reserved for
persistence and are never used for component commands or snapshots.

## Commands

```text
command <version> <source> <requestId> <name> <fields...>
```

The current commands are `component.attach`, `component.detach`,
`eq.set_parameter`, `eq.set_bypass`, `eq.reset_filter`, `eq.add_bank`,
`eq.remove_bank`, `eq.rename_bank`, `eq.select_bank`, `analyzer.listen`,
`fit.start`, `fit.cancel`, `fit.clear`, `fit.complete`, and `fit.fail`. All EQ
values are absolute. `fit.complete` frames its filters and values with explicit
counts and is committed atomically by Host. `eq.add_bank` accepts an optional
single name atom; when omitted, Host generates the deterministic bank name.

## Events

```text
event <version> host <eventId> <name> <fields...>
```

Events include `host.initialized`, `component.attached`, `store.updated`,
`command.rejected`, and `operation.changed`.

## EQ Snapshot

```text
snapshot <version> host eq <revision> <selectedBank> <bankCount>
    <bankId> <bankName> <filterCount>
        <filterId> <bypass> <valueCount> <absoluteValue...>
```

Bank and filter IDs are one-based. Counts frame every variable-length section.
The same atom sequence may arrive through Max as a `snapshot` selector or as a
`list` whose first atom is `snapshot`; these are two Max deliveries of one
protocol message, not two protocol formats.

Filter definitions use a second snapshot family:

```text
snapshot <version> host definitions <revision> <filterCount>
    <filterId> <type> <defaultBypass> <parameterCount>
        <name> <minimum> <maximum> <scale> <defaultValue>
```

## Persistence

Persistence is the only Dictionary boundary. `MaxDictionarySerializer` converts
the persisted typed state to and from a Max Dictionary while the device owns
the dictionary lifetime. The Max transport message is `dictionary <name>` and
must reach `pattrstorage` synchronously; only state preparation may be
debounced, never the temporary dictionary name.
