# General Formatting

Общие правила форматирования и именования для `InteropSandbox`. Языкоспецифичные правила находятся в отдельных документах:

- [`CppFormatting.md`](CppFormatting.md) — C++;
- [`CSharpFormatting.md`](CSharpFormatting.md) — C#;
- [`JavaScriptFormatting.md`](JavaScriptFormatting.md) — JavaScript в Max runtime.

## Общие правила

- Использовать 4 пробела. Табуляция в исходном коде не используется.
- Использовать Allman braces: открывающая скобка находится на новой строке.
- Всегда использовать `{}` для управляющих конструкций.
- Одна инструкция на строку.
- Пустая строка отделяет логические блоки, но не отдельные инструкции.
- Не выравнивать код большими последовательностями пробелов.
- Мягкая граница длины строки — около 120 символов. Это не жёсткое ограничение.
- Предпочитать небольшие методы с одной ответственностью.
- Предпочитать guard clauses вместо глубокой вложенности.
- Не создавать abstraction layer без конкретной необходимости.
- Не хранить закомментированный старый код.
- Не сохранять compatibility code для удалённой архитектуры без явной причины.
- Комментарии объясняют причины, ownership, lifetime, threading и ограничения. Они не пересказывают очевидный код.

## Выбор языка и слоя

Правила форматирования не заменяют архитектурные правила. Для выбора места реализации использовать [`Rules.md`](Rules.md):

- C++ — Max/min-api integration, native interop, realtime boundary и native DSP;
- C# — Managed Core, state, coordination, analysis и protocol behavior;
- JavaScript — Max UI, bindings, presentation и пользовательские intents.

Не переносить архитектурные, lifecycle, ABI или realtime-правила в style guide, если для них уже есть отдельная документация.

## Tooling

Механические правила по возможности поддерживать настройками:

- C# — `.editorconfig`, Visual Studio, `dotnet format`;
- C++ — `.clang-format`;
- JavaScript — project-specific formatter rules.

Formatter не определяет architecture, ownership, threading, lifetime, ABI или naming semantics. Если formatter конфликтует с осмысленным project rule, сначала исправить конфигурацию formatter-а.
