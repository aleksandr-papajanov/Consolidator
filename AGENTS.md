# Experimental architecture policy

This repository is in active experimental development. Do not preserve or add
backward compatibility unless the user explicitly requests it.

Never add compatibility aliases, migration adapters, fallback paths, legacy
branches, deprecated wrappers, or duplicate APIs merely to keep old code
working. When the architecture changes, update all callers and delete the old
model directly.

If a requested change would cause an unusually destructive or externally
irreversible action, report that fact and ask for confirmation. For ordinary
code refactors, proceed directly with the clean target design.

Do not build, run tests, or perform other verification unless the user
explicitly asks for it. Do not announce an intended action and then stop
before completing the current request. Continue the requested work until its
scope is finished; do not pause for intermediate confirmation.
