# Topology indexes and observers

## Topology lifecycle

`TopologyIndex` lives in the shared `Core/Topology` scope. It is a derived view
of bank-group and focused-bank state used by both state observers and Routing;
neither consumer owns the implementation.

`StateTopologyObserver` owns topology lifecycle:

- adding an instance indexes its current bank groups and materializes all peer
  components once after the complete state tree is attached;
- changing a group moves the bank between group buckets;
- removing an instance removes its buckets before surviving peers refresh.

The group value is bank-owned for notification routing but has local edit
scope. Changing membership rebuilds peer buckets; it is not itself propagated
as a grouped DSP edit.


## Membership commands

Topology user intents use these bodies:

```text
group_banks bankCount targetInstanceId bankIndex [targetInstanceId bankIndex]...
ungroup_banks targetInstanceId bankIndex
clear_topology
```

`group_banks` requires at least two ungrouped banks from distinct instances.
Managed validates the complete selection, allocates the first free user group
in `1..15` (zero is automatic), and commits every membership in one history
transaction. `ungroup_banks` resolves all members of the target bank's user
group in Managed; group zero cannot be dissolved. Clear removes user groups.
These commands own their history point; frontend history wrappers are absent.

`bankIndex` is zero-based and must be in the range `0..6`. The target bank group
is resolved for the explicit target instance at that index; a missing target or
bank context is rejected without falling back to the source selection.

State changes are published by the existing
observer chain and addressed by `StateChangeRouter` using topology and focus.
State writes pass through registered path-scoped policies before the common
typed write validation and history transaction. Bank-group writes are checked
by the bank-group policy: automatic group `0` is immutable, a bank cannot be
assigned to a second group, and one track cannot contribute multiple banks to
the same group. Rejected policy writes do not open or commit a history
transaction.
