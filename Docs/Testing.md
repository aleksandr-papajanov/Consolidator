# Testing

## Цель

Тестовая система доказывает наблюдаемые контракты крупных компонентов и
их границ. Количество tests и coverage percentage не являются самостоятельной целью.

Основная единица тестирования — use case:

```text
given system state and registered instances
→ public command or user intent
→ decoding and routing
→ authoritative state/lifecycle transition
→ DSP publication and protocol callback
→ observable client result
```

Микро-тест допустим только когда сам малый контракт является критичной
границей: ABI layout, atom representation, pointer lifetime или realtime publication.
Очевидные getters, private helpers и случайная структура кода отдельно не тестируются.

## Уровни

### Managed component/use-case

`Tests/Managed/` собирает production DI graph в изолированном provider. Тест подаёт
реальный `ProtocolInput` и проходит весь Managed path:

```text
CommandDecoder → endpoint → router → command handler
→ state/history/topology → DSP publisher + ProtocolOutput
```

`UseCases/` хранит command workflows, `Contracts/` — только критические ABI
контракты, `Support/` — общие fixtures. Тесты не вызывают command handlers напрямую.

### Native component/integration

`Tests/Native/Contracts/` сравнивает fixed ABI layout с Managed contract.
`Tests/Native/Realtime/` использует production DSP consumer и проверяет
claim/copy semantics triple-buffer exchange без загрузки Max runtime. Native runner должен
оставаться standalone и не ссылаться на `MaxAPI.dll`.

### Published NativeAOT integration

`Tests/Integration/NativeAot/` загружает опубликованную `Consolidator.Managed.dll`,
вызывает exports через `cdecl`, передаёт independently declared ABI atoms и
немедленно копирует callback frames. Этот уровень доказывает:

- DLL load и полный export surface;
- register/unregister и audio handle;
- initial DSP snapshot;
- Native atoms → Managed command/state → native callback atoms;
- unregister barrier.

Integration project не ссылается на Managed assembly, иначе он бы не доказывал
опубликованную ABI boundary.

### Max JavaScript use cases

`Consolidator.Max/tests/` загружает production JavaScript в Max-compatible environment и проверяет:

- UI intent → exact protocol command frame;
- Managed callback-shaped frame → client/ViewModel/presentation state;
- transactions, registry, observed target, bank grouping и lifecycle;
- целостность runtime paths всего Max package.

JS suite и NativeAOT suite используют один wire contract с двух сторон. Запуск самого
`.mxe64` в Max, включая фактический `t_atom → NativeAtom` в `AtomCodec`, требует Max host и
не подменяется Node или xUnit harness.

## Обязательные use cases

- initialize и observe target;
- write, read, reset и malformed command;
- grouped-bank propagation и notification routing;
- registry snapshot и registry revision handling;
- begin/end history и jump;
- register, DSP publication, unregister barrier;
- JS control gesture и transaction lifecycle;
- Max package dependency resolution.

### Performance and lifecycle regression matrix

Managed suites must cover coalesced parameter and grouped-bank edits, stale
analyzer revision suppression, exclusive instance activity targeting, bounded
control queue
overload, and unregister cancellation. Registry tests must cover typed deltas,
revision-gap recovery, and the fact that adding an instance does not require a
full snapshot for existing clients.

Native suites must cover bounded analysis queue replacement, lossless control
frames and control priority, bounded drain batches, repeated queue scheduling,
teardown with pending work, and the allocation-free audio ring including wrap
and overflow behavior.

JavaScript suites must cover analyzer-only FFT presentation, Equalizer-only
curve delivery, one redraw per FFT update, instance activity changes with Live
selection
or inactive, targeted registry row deltas, single revision-gap resync, latest
gesture value preservation, and subscription/demand cleanup on destroy.

The Max/Ableton stress pass uses 1, 8, 16, and 32 instances. For each size it
loads the project, opens and closes the UI, performs sustained EQ drags, bank
switches, grouped edits, undo/redo, device add/remove, and 10-15 minutes of
continuous playback. Record queue depth, control latency, analyzer activity,
and Managed/native metrics before and after playback; queue depth and latency
must remain stable.

Один факт не дублируется на всех уровнях. Например, Managed test проверяет routing
и state semantics, а NativeAOT integration — marshaling той же команды через реальную DLL.

## Запуск

Из корня repository в Developer PowerShell for Visual Studio:

```powershell
dotnet test Tests/Managed/Consolidator.Managed.Tests.csproj --configuration Release

& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Tests/Native/Consolidator.Native.Tests.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
& "Tests/Native/build/Release/Consolidator.Native.Tests.exe"

& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Consolidator.Managed/Consolidator.Managed.csproj /t:Publish /p:PublishProfile=NativeAndManaged /p:Configuration=Release /p:Platform=x64 /p:RuntimeIdentifier=win-x64 /p:SelfContained=true /p:PublishAot=true /p:NativeLib=Shared /p:PublishSingleFile=false /p:BuildNativeBeforePublish=true
dotnet test Tests/Integration/Consolidator.Integration.Tests.csproj --configuration Release

node Consolidator.Max/tests/ClientTests.js
```

Нельзя запускать Integration на старой DLL. Каждый full verification сначала выполняет
combined Native + Managed publish.
