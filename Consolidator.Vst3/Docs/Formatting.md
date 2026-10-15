# VST3 formatting

Обязательны [общие правила](../../Docs/Formatting.md) и единые
[C++ conventions](../../Docs/Formatting.md#c-conventions).

Namespaces соответствуют `Host`, `Features` и `Shared` из
[Architecture](Architecture.md). Методы JUCE overrides сохраняют имена API (`paint`, `resized`,
`processBlock`); новые собственные методы используют проектный PascalCase.

Стили контролов — [Theme](Aspects/Theme.md); обязанности widgets, ViewModels,
bindings и clients — [Rules](Rules.md) и [FrontendComposition](Aspects/FrontendComposition.md).
