# VST3 build и publish

Все команды требуют явного разрешения на сборку/публикацию, см.
[AGENTS.md](../../../AGENTS.md). Выполнять из корня репозитория в Developer
PowerShell for Visual Studio. Для MSBuild использовать:

```text
C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe
```

## Основной workflow

У Managed в Visual Studio Publish один профиль — `Vst3Plugin`. Publish:

1. Публикует Managed NativeAOT и зависимости.
2. Копирует `Consolidator.Managed.dll` и `nlopt.dll` из NativeAOT publish output
   в `Consolidator.Vst3/build/managed-runtime`.
3. Вызывает VST3 makefile project через Visual Studio MSBuild/CMake.
4. Копирует staged runtime в bundle и устанавливает его в настроенную системную
   VST3-папку, даже если VST3 binary уже up-to-date.

Не создавать отдельный publish-output
и не подменять combined workflow обычным `dotnet publish`: нужны Visual Studio
C++ targets. `SkipVst3Build=true` используется только для Managed NativeAOT перед
integration tests; output остаётся в том же `managed-runtime`.

`Consolidator.Vst3.vcxproj` делегирует сборку preset в CMake. Preset требует
`Consolidator.Managed.dll` и `nlopt.dll` в `CONSOLIDATOR_MANAGED_RUNTIME_DIR`.
CMake обновляет runtime внутри bundle перед каждой установкой; копирование не
зависит от post-build шага линкера, который пропускается при up-to-date VST3
binary. `ConsolidatorVST3Install` копирует bundle в
`CONSOLIDATOR_VST3_INSTALL_DIR`, по умолчанию стандартную Windows x64 VST3-папку
под `CommonProgramW6432`. Ошибка установки должна завершать publish с ошибкой.

## Только VST3 с уже опубликованным runtime

Из `Consolidator.Vst3` в Developer PowerShell:

```powershell
cmake --preset vs2026-x64
cmake --build --preset vs2026-release-install
```

Первый configure получает JUCE, если FetchContent source не предоставлен извне.
Такой build не обновляет Managed. При изменении ABI/protocol/state совместно
публиковать Managed и VST3; старая DLL не подтверждает новую интеграцию.
Сборка и установка не подтверждают host lifecycle или поведение UI в Live.
