# Consolidator — инструкции агенту

## Перед работой

1. Прочитать [корневой README](README.md) и README затрагиваемых проектов:
   [Managed](Consolidator.Managed/README.md), [Native](Consolidator.Native/README.md),
   [VST3](Consolidator.Vst3/README.md), [EmulationLab](EmulationLab/README.md).
2. Для решения и каждого затрагиваемого проекта прочитать `Docs/Rules.md`,
   `Docs/Formatting.md` и `Docs/Architecture.md`. README — оглавление и краткая
   суть; нормативная ответственность находится в Rules, технические связи — в Architecture.
3. Из Architecture перейти к нужным `Docs/Aspects/<Aspect>.md` и проследить
   существующий путь данных. Если поведение не понятно, сначала читать его
   контракт, затем исследовать код; не выбирать другой слой ради удобства.

Подход к документации, структура scope, атомарность и сопровождение определены
в [общих правилах](Docs/Rules.md#документация-scope).

## Проектные skills

Правила реализации действуют всегда и находятся в Docs соответствующего scope.
Skills используются для отдельных процедур по задаче пользователя:

| Задача | Skill |
| --- | --- |
| Исследовать референс и подобрать модель | [consolidator-dsp-emulation](.agents/skills/consolidator-dsp-emulation/SKILL.md) |
| Проверить и исправить уже написанные изменения | [consolidator-change-review](.agents/skills/consolidator-change-review/SKILL.md) |
| По явному запросу написать полезные тесты | [test-changes](.agents/skills/test-changes/SKILL.md) |

Обычная реализация или ревью не запускают workflow написания тестов.
Разрешения на Git, сборку и запуск проверок определяются ниже, а не фактом
наличия или применения skill.

## Запрещённые действия без разрешения

- Не собирать и не публиковать проект.
- Не запускать тесты или другие проверки.
- Не создавать коммиты.
- Не выполнять любые команды или операции Git, включая status и diff.

Эти действия разрешены только после явного запроса или разрешения пользователя.
Уже данное разрешение действует в пределах своего scope; не спрашивать повторно.
Обновление документации или применение skill само по себе разрешений не выдаёт.

Для DSP действуют [Native DSP rules](Consolidator.Native/Docs/Rules.md):
запрещены создание и запуск автоматических тестов аудиоалгоритмов, DSP-примитивов
и runtime graph. Ручное исследование/прослушивание через EmulationLab или Plugin
Doctor — только по явному запросу. ABI, protocol, persistence и transport tests
имеют отдельный scope.

## Изменения

Следовать [общим правилам](Docs/Rules.md): без compatibility/fallback/legacy
веток и duplicate APIs; удалять заменённый код и опровергнутые гипотезы.
Подтверждённые ошибки исправлять; assertions не ослаблять ради прохождения теста.

Если пользователь явно просит коммит, включать все текущие изменения рабочего
дерева, незатреканные файлы и артефакты, без самостоятельного отбора.

## Разрешённая сборка и публикация

Актуальные Visual Studio MSBuild path, `Vst3Plugin` profile, CMake и install
workflow находятся в [VST3 Build](Consolidator.Vst3/Docs/Aspects/Build.md).
Не восстанавливать старые publish profiles или отдельный Managed output.
