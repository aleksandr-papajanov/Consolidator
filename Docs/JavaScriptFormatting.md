# JavaScript Formatting

Правила форматирования и именования JavaScript кода в `Consolidator.Max/js`.

## Runtime

Production-код загружается только объектами `v8`/`v8ui` и использует
современный синтаксис: `const`/`let`, classes, destructuring, rest/spread и
arrow functions. `var`, prototype-конструкторы и legacy `include` запрещены.

Использовать 4 пробела, Allman braces, `{}` для control flow и `;` после каждой инструкции. Не добавлять polyfills или runtime dependencies без необходимости.

## Files and dependencies

Каждый файл должен иметь одну основную сущность. Тип и его методы находятся в
одном class declaration:

```javascript
class ObservableValue
{
    constructor(initialValue)
    {
        this.value = initialValue;
    }

    set(value)
    {
        this.value = value;
    }
}
```

Не рассчитывать на случайный порядок загрузки файлов.

V8 libraries use CommonJS `require` and explicit named exports:

```javascript
const { ObservableValue } = require("../ViewModels/ObservableValue.js");

module.exports = {
    ObservableValue: ObservableValue
};
```

V8 modules не публикуют constructors или helpers через глобальную область.

Max entrypoints являются исключением: selectors и lifecycle hooks,
вызываемые Max (`anything`, `paint`, `onclick`, `notifydeleted` и т. п.),
объявлять как глобальные named function declarations. Всю stateful-логику
держать в class instance, а глобальную function оставлять тонким adapter.

## Naming

| Сущность | Стиль | Пример |
| --- | --- | --- |
| Constructor / type | `PascalCase` | `NativeProtocolClient` |
| Method | `camelCase` | `handleControl()` |
| Local variable / parameter | `camelCase` | `requestId` |
| Field | `camelCase` | `destroyed` |
| Constant | `UPPER_SNAKE_CASE` | `PROTOCOL_VERSION` |
| Plain object mapping | `PascalCase` | `ConsolidatorControlMapping` |
| File | `PascalCase` | `NativeProtocolClient.js` |

Protocol selectors и Max control names сохранять буквально: они являются внешним contract.

## Callbacks and lifecycle

Если callback должен обращаться к instance, использовать arrow function:

```javascript
this.unsubscribe = store.subscribe((value) =>
{
    this.apply(value);
});
```

Каждый subscribe должен иметь соответствующий unsubscribe. Components с subscriptions, tasks или child components должны иметь idempotent `destroy()`:

```javascript
destroy()
{
    if (this.destroyed)
    {
        return;
    }

    this.destroyed = true;
    this.unsubscribers.forEach((unsubscribe) =>
    {
        unsubscribe();
    });
    this.unsubscribers = [];
}
```

После `destroy()` callbacks не должны обращаться к освобождённым dependencies.

## Protocol and UI layers

- Проверять selector и минимальную длину `args` до чтения позиций массива.
- Optional message arrays нормализовать на границе через `args = args || []`.
- Идентификаторы сравнивать через `String(...)`, если Max может передать numeric и string representations.
- Malformed input безопасно игнорировать, если это не нарушает обязательный protocol invariant.
- Не дублировать protocol validation в bindings, если она уже выполнена в client.
- Request callback удалять из `pending` до его вызова, чтобы callback мог безопасно инициировать новый request.

Текущие роли слоёв:

```text
Client      transport and request completion
ViewModel   UI-facing state
Presenter   presentation values
Binding     UI intent and presentation
Controller  feature composition and lifecycle
```

Bindings не знают деталей transport. ViewModels не обращаются напрямую к Max UI objects. Controllers владеют lifecycle feature dependencies.

## Request lifecycle

Protocol clients должны очищать завершённые requests и сбрасывать callbacks при `destroy()`:

```javascript
const callback = this.pending[String(requestId)];
if (!callback)
{
    return;
}

delete this.pending[String(requestId)];
callback(response);
```

`destroy()` должен сделать дальнейшую отправку сообщений безопасной, например заменить transport function на no-op и очистить handlers/pending requests.

## Tests

Тестовые файлы используют тот же runtime-compatible синтаксис, что и product code. Общие suites подключаются через `require`, а не через browser-specific module loaders.

Тестировать message handling, request completion, subscriptions, `destroy()`, malformed input и ViewModel/Binding behaviour.

Если тесты разделены на suites, общий entrypoint подключает их через `require`:

```javascript
require("./suites/client/ClientSuite.js");
```

