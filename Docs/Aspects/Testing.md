# Testing

## DSP исключён

Автотесты DSP-процессоров, примитивов и runtime graph запрещены. См. [правила DSP](../../Consolidator.Native/Docs/Rules.md).
EmulationLab/Plugin Doctor используются только для явно запрошенных ручных исследований и прослушивания.
Тесты protocol, ABI, persistence, Managed lifecycle и транспорта сохраняются.

## Цель

Workflow написания новых тестов запускается по явному запросу пользователя.
Обычная реализация или ревью сами по себе его не запускают. Порядок выбора и
реализации сценариев — [test-changes](../../.agents/skills/test-changes/SKILL.md).
Запрос написать тесты не разрешает автоматически сборку, публикацию или запуск:
действуют [разрешения AGENTS.md](../../AGENTS.md).

Тестовая система доказывает наблюдаемые контракты крупных компонентов и
их границ. Количество tests и coverage percentage не являются самостоятельной целью.

### Обработка failures

Падение теста считается сигналом о проблеме, которую необходимо исследовать и
исправить. Сначала нужно установить root cause: production-регрессию, ошибку
тестовой инфраструктуры или fixture, нарушение контракта, гонку либо неверную
изоляцию состояния. Нельзя менять assertion, ослаблять проверку или удалять
тест только для того, чтобы suite стал зелёным. Тест изменяется только после
подтверждения намеренного изменения наблюдаемого контракта и должен описывать
новое требуемое поведение.

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

### Сценарии и fixtures

Выбирать сценарии по существенным рискам изменения и пробелам имеющегося покрытия,
а не по числу строк или веток. Не повторять production-вычисление в assertion:
ожидаемый результат следует из наблюдаемого контракта. Тест должен допускать
рефакторинг, сохраняющий этот контракт.

Не подменять mock-объектом именно тот router, state mechanism или transport,
чью работу должен доказать сценарий. Переиспользовать существующие runners,
frameworks и fixtures; не добавлять production API только ради private access.
Изолировать instances/services, управлять async completion через явные сигналы
и существующие barriers вместо sleeps и случайных задержек. Граничные случаи
добавлять по реальным рискам, не превращать их в обязательную матрицу каждой правки.

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
Native runner содержит только ABI, persistence blob и restore-gate contracts.
DSP Realtime suites удалены. Он остаётся standalone и не требует DAW для ABI/transport checks.

### Published NativeAOT integration

`Tests/Integration/NativeAot/` загружает опубликованную `Consolidator.Managed.dll`,
вызывает exports через `cdecl`, передаёт independently declared ABI atoms и
немедленно копирует callback frames. Этот уровень доказывает:

- DLL load и полный export surface;
- register/unregister и audio handle;
- initial DSP snapshot;
- Native atoms → Managed command/state → native callback atoms;
- unregister barrier;
- остановку и повторное создание Managed services без выгрузки NativeAOT DLL.

Integration project не ссылается на Managed assembly, иначе он бы не доказывал
опубликованную ABI boundary.

### VST3 host и presentation

JUCE/DAW lifecycle и UI use cases описаны в [VST3 Validation](../../Consolidator.Vst3/Docs/Aspects/Validation.md). Component fixtures должны повторять реальную форму входов и callback completion; они не подменяют host validation.

## Обязательные use cases

- initialize и observe target;
- write, read, reset и malformed command;
- grouped-bank propagation и notification routing;
- local/group Bank Commit approximation, source reset и atomic failure;
- explicit instance/group mute и exclusive/additive solo targeting;
- registry snapshot и registry revision handling;
- begin/end history и jump;
- register, DSP publication, unregister barrier;
- UI control gesture и transaction lifecycle;
- VST3 runtime DLL discovery и host lifecycle.

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

DSP telemetry coverage must prove stable shared-snapshot reads, active-source
publication gating, latest-only client revisions, control read-only state while
automation is active, and an independent final slow-level commit for every
target instance when grouped automation is disabled.

UI coverage включает latest target/registry/telemetry revisions, snapshot batching, coalesced gestures и teardown. Разрешённый host stress pass и диагностика текущего frontend — в [VST3 Validation](../../Consolidator.Vst3/Docs/Aspects/Validation.md).

Один факт не дублируется на всех уровнях. Например, Managed test проверяет routing
и state semantics, а NativeAOT integration — marshaling той же команды через реальную DLL.

## Запуск

Из корня repository в Developer PowerShell for Visual Studio:

```powershell
dotnet test Tests/Managed/Consolidator.Managed.Tests.csproj --configuration Release

& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Tests/Native/Consolidator.Native.Tests.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
& "Tests/Native/build/Release/Consolidator.Native.Tests.exe"

& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Consolidator.Managed/Consolidator.Managed.csproj /t:Publish /p:PublishProfile=Vst3Plugin /p:SkipVst3Build=true
dotnet test Tests/Integration/Consolidator.Integration.Tests.csproj --configuration Release



```

EmulationLab self-test и CTest регистрации удалены; ручной workflow описан в [DSP/EmulationLab](../../EmulationLab/Docs/Aspects/Usage.md).

Нельзя запускать Integration на старой DLL. Параметр `SkipVst3Build=true`
публикует актуальный Managed NativeAOT runtime в
`Consolidator.Vst3/build/managed-runtime`, откуда integration fixture загружает
DLL. Этот режим не собирает и не устанавливает VST3; обычный Publish через
профиль `Vst3Plugin` также собирает и устанавливает плагин.
