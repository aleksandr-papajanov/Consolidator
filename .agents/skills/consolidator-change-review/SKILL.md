---
name: consolidator-change-review
description: Review InteropSandbox changes against project architecture, documentation, realtime constraints, performance requirements, and code quality rules. Use after implementing a feature or refactor, before considering the work complete.
---

# InteropSandbox Change Review

Perform a deep architectural and implementation review of the current changes.

The purpose of this skill is not to rewrite code or add abstractions blindly. Determine whether the implementation fits the current sandbox architecture, follows its documented contracts, and solves the requested problem without speculative fallback behavior.

Review for:

- correct separation between Max, Native, Managed, and JavaScript layers;
- unnecessary complexity or duplicated concepts;
- ownership, lifetime, callback, and unregister-barrier problems;
- unnecessary allocations, copies, locks, queues, rebuilds, or scheduler work;
- violations of realtime/audio-thread requirements;
- unnecessary Max/Managed/JavaScript communication;
- ABI, protocol, state synchronization, concurrency, and lifecycle bugs;
- changes that are based only on an unconfirmed hypothesis and should be removed.

Do not assume the implementation is correct merely because tests pass.

## 1. Understand the project first

Before reviewing the diff in detail, read the relevant project documentation and rules:

- `InteropSandbox/AGENTS.md`;
- `InteropSandbox/Docs/README.md`;
- `InteropSandbox/Docs/Rules.md`;
- `InteropSandbox/Docs/Formatting.md` and the language-specific formatting document;
- `InteropSandbox/Docs/ManagedNativeCommunication.md` when the change crosses the ABI or callback boundary;
- testing documentation and documentation near the modified subsystem.

Apply the most specific `AGENTS.md` rules to every reviewed file. Do not inspect or modify documentation outside `InteropSandbox` for an InteropSandbox review.

Create a short internal model of:

- Native ownership of Max integration, ABI conversion, callbacks, queues, and realtime DSP;
- Managed ownership of application state, coordination, analysis, persistence, and protocol behavior;
- JavaScript ownership of Max UI bindings and presentation;
- authoritative state and communication direction;
- callback and object lifetimes;
- realtime boundaries and thread ownership;
- test boundaries between Managed, Native, and Integration projects.

Do not propose changes before understanding these rules.

## 2. Inspect the complete change

Use the actual current change as the primary scope. Review staged, unstaged, and untracked files when Git inspection is explicitly permitted by `InteropSandbox/AGENTS.md` and the user. Otherwise use the editor's diff and direct file inspection without running Git commands.

Determine:

1. What problem the change is intended to solve.
2. Which architecture or contract it modifies.
3. Which existing path it replaces.
4. Whether obsolete code or a duplicate path remains.
5. Whether the change expanded beyond the requested behavior.
6. Whether any attempted fix was disproven and should be reverted instead of retained as a fallback.

Follow the data flow end-to-end where a local test could give false confidence:

```text
Max external
→ Native ABI/bridge
→ Managed exports and core
→ Native callback/queue
→ Max thread or outlet
```

For UI changes also follow:

```text
Managed protocol
→ Max JavaScript client
→ ViewModel/presenter/binding
→ Max control
```

## 3. Review priorities

Report findings first, ordered by severity and grounded in file references. Focus on concrete defects and risks:

- ABI layout or calling-convention mismatches;
- callbacks that can outlive their native context;
- unregister or destruction races;
- borrowed pointers retained after callback return;
- Max calls from the wrong thread;
- allocation, blocking, logging, FFT, or application logic on realtime paths;
- authoritative state violations or lazy remote hydration;
- duplicate compatibility/fallback paths;
- protocol or export mismatches;
- tests that pass locally while missing the changed boundary.

For each finding, explain the observable consequence and the smallest corrective direction. Do not preserve an unconfirmed diagnostic patch merely because it might help in another scenario. Preserve fixes for confirmed defects.

If no issues are found, say so clearly and list remaining test or host-verification gaps.

## 4. Verification

Run builds, tests, or Git commands only after explicit user permission. For authorized verification, use the InteropSandbox workflow:

- full build: `InteropSandbox/.vscode/build-all.cmd`;
- Managed tests: `dotnet test InteropSandbox/Tests/Managed/Consolidator.Managed.Tests.csproj --configuration Release`;
- Native tests: build the Native test target with the documented Visual Studio MSBuild workflow, then run its executable;
- Integration tests: publish the Managed NativeAOT library with the documented `NativeAndManaged` Visual Studio MSBuild command, then run `dotnet test InteropSandbox/Tests/Integration/Consolidator.Integration.Tests.csproj --configuration Release`.

Do not substitute `dotnet publish` for the combined Native + Managed publish: the Native VC++ project requires Visual Studio MSBuild targets.

Report exact commands and results. Do not claim verification when only files were inspected.
