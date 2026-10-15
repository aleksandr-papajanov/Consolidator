# Native formatting

Обязательны [общие правила](../../Docs/Formatting.md) и единые
[C++ conventions](../../Docs/Formatting.md#c-conventions).

DSP и Transport сохраняют собственные namespace/ownership
границы из [Runtime](Aspects/Runtime.md) и [InteropRules](../../Docs/Aspects/InteropRules.md).
Header задаёт контракт, `.cpp` — реализацию; короткий hot-path inline допустим
по правилам [Implementation](Aspects/Implementation.md).

Форматирование не разрешает application logic в Native. Realtime/lifetime
определяются [InteropRules](../../Docs/Aspects/InteropRules.md) и [DSP rules](Rules.md).
