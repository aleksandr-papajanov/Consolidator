---
name: test-changes
description: Review InteropSandbox rules and current changes, then create focused tests for the observable behavior introduced or changed by the diff.
---

# Test Changed Functionality

Use this skill when implementation changes already exist and need focused regression coverage.

The goal is to prove changed contracts, not merely increase coverage or make the suite pass. Do not add tests for speculative behavior, private implementation details, or a diagnostic hypothesis that was not confirmed.

## Core rules

- Read the InteropSandbox rules and relevant documentation before writing tests.
- Treat the current change as the primary scope.
- Test observable behavior and public boundaries.
- Prefer a small number of meaningful deterministic tests.
- Reuse existing test projects, helpers, assertions, and conventions.
- Do not introduce a new test framework.
- Do not rewrite production code unless a minimal testability change is genuinely required.
- Do not change production behavior merely to make a test easier to write.
- Do not use sleeps, arbitrary timing delays, network calls, external services, or visual-pixel assertions without existing project infrastructure.
- Respect the rule that a failed hypothesis must be removed rather than retained as a fallback or extra defensive path.

## 1. Read project rules

Before creating tests, read the relevant files inside `InteropSandbox`:

- `AGENTS.md`;
- `Docs/README.md`;
- `Docs/Rules.md`;
- `Docs/Formatting.md` and the language-specific formatting document;
- `Docs/ManagedNativeCommunication.md` for ABI, callback, queue, ownership, lifecycle, or realtime changes;
- documentation near the changed subsystem.

Do not inspect or modify files outside `InteropSandbox` for this task unless the user explicitly asks for it.

Pay particular attention to:

- Native/Managed/Max architecture boundaries;
- authoritative state ownership;
- ABI and protocol contracts;
- callback, queue, unregister, and destruction lifetime rules;
- realtime/audio-thread restrictions;
- test project responsibilities and allowed dependencies.

## 2. Understand the existing test architecture

Use the existing projects and levels:

- `Tests/Managed/` — xUnit component tests for atom decoding, protocol behavior, state, and managed lifecycle;
- `Tests/Native/` — standalone C++ tests for ABI structs, atom conversion, and native ownership/lifetime helpers;
- `Tests/Integration/` — xUnit tests for the published Managed NativeAOT library, exports, and the Managed/Native contract.

Read representative tests near the changed behavior before adding new ones. Reuse existing setup and assertions. Do not duplicate the same assertion at multiple levels unless each test proves a different boundary.

For JavaScript changes, inspect the existing Max JavaScript test setup and keep tests compatible with the supported Max runtime.

## 3. Inspect the current change

Review staged, unstaged, and untracked files when Git inspection is explicitly permitted by `AGENTS.md` and the user. Otherwise use the editor's diff and direct file inspection without running Git commands.

Identify:

1. Which observable behaviors changed.
2. Which contracts were added, removed, or tightened.
3. Which boundaries are crossed: Managed, Native ABI, callback/queue, realtime audio, Integration, or Max JavaScript.
4. Which regression risks matter most.
5. What existing tests already prove and what precise gap remains.

Follow the changed data flow far enough to avoid a false-local test:

```text
Max input
→ Native conversion
→ Managed export/core
→ Native callback and queue
→ Max output
```

## 4. Define the smallest sufficient test set

Define each test contract before writing assertions:

- Given: the relevant state, input, or ABI/protocol frame;
- When: the public operation or lifecycle event occurs;
- Then: the observable output, state transition, callback, export result, or rejection.

Choose the narrowest level that proves it:

- Managed component behavior in `Tests/Managed`;
- Native ABI/ownership behavior in `Tests/Native`;
- published export or cross-language behavior in `Tests/Integration`;
- Max JavaScript protocol behavior in its existing JavaScript test suite.

Cover edge cases only when they are part of the changed contract, especially:

- malformed or empty input;
- unknown atom types or invalid instance IDs;
- source/instance isolation;
- registration, unregister, and destruction lifecycle;
- borrowed-pointer ownership and callback lifetime;
- queue coalescing versus frame retention;
- stale or unchanged state;
- realtime safety and absence of forbidden work on the audio path.

Avoid assertions about private fields, incidental call order, exact allocation counts, or implementation structure unless explicitly documented as a contract.

## 5. Implement focused tests

Follow the local test style:

- use xUnit in Managed and Integration projects;
- use the existing standalone C++ test runner and assertions in Native tests;
- use sentence-style test names describing one behavior;
- keep setup deterministic and local;
- keep protocol assertions separate from derived state assertions;
- do not add timing sleeps or fragile ordering assumptions;
- keep integration tests focused on the published NativeAOT boundary.

If the behavior is not testable through a public seam, first inspect existing fixtures or helpers. Make a minimal production change only when it improves a real public testability boundary without changing behavior.

Do not weaken assertions to accommodate an uncertain implementation. Return to the documentation and production data flow when the expected contract is unclear.

## 6. Verify and report

Run build or test commands only after explicit user permission. For authorized verification, use the documented InteropSandbox workflows:

- full build: `InteropSandbox/.vscode/build-all.cmd`;
- Managed tests: `dotnet test InteropSandbox/Tests/Managed/Consolidator.Managed.Tests.csproj --configuration Release`;
- Native tests: use the documented Visual Studio MSBuild target and run the resulting Native test executable;
- Integration tests: first publish through the Visual Studio `MSBuild.exe` `NativeAndManaged` workflow, then run `dotnet test InteropSandbox/Tests/Integration/Consolidator.Integration.Tests.csproj --configuration Release`.

Do not use plain `dotnet publish` for the combined workflow because it cannot import the Native VC++ targets.

Before reporting, review the resulting diff and confirm:

- each added test covers a changed observable contract;
- no test duplicates another level without a distinct boundary;
- test projects and targets are registered correctly;
- documentation was updated when the contract or workflow changed.

Report:

- behaviors covered;
- exact test files and test names;
- commands run and results;
- tests not run and why;
- remaining risk requiring host or manual verification.

Do not claim a behavior is verified when only the test file was inspected or execution was unavailable.
