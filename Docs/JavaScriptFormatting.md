# JavaScript Formatting

Правила форматирования и именования JavaScript кода в `Consolidator.Max/js`.

## Runtime

Код выполняется в Max runtime и использует ES5-compatible стиль. Не использовать `let`, `const`, arrow functions, classes, modules, destructuring и spread syntax без подтверждённой поддержки текущего runtime.

Использовать 4 пробела, Allman braces, `{}` для control flow и `;` после каждой инструкции. Не добавлять polyfills или runtime dependencies без необходимости.

## Files and dependencies

Каждый файл должен иметь одну основную сущность. Конструктор и его prototype methods обычно находятся в одном файле:

```javascript
function ObservableValue(initialValue)
{
    this.value = initialValue;
}

ObservableValue.prototype.set = function (value)
{
    this.value = value;
};
```

Dependencies подключаются в начале файла через Max `include`:

```javascript
include("Project:/js/Clients/NativeProtocolClient.js");
include("Project:/js/ViewModels/ObservableValue.js");
```

Не рассчитывать на случайный порядок загрузки файлов.

## Naming

| Сущность | Стиль | Пример |
| --- | --- | --- |
| Constructor / type | `PascalCase` | `NativeProtocolClient` |
| Prototype method | `camelCase` | `handleControl()` |
| Local variable / parameter | `camelCase` | `requestId` |
| Field | `camelCase` | `destroyed` |
| Constant | `UPPER_SNAKE_CASE` | `PROTOCOL_VERSION` |
| Plain object mapping | `PascalCase` | `ConsolidatorControlMapping` |
| File | `PascalCase` | `NativeProtocolClient.js` |

Protocol selectors и Max control names сохранять буквально: они являются внешним contract.

## Callbacks and lifecycle

Если callback должен обращаться к instance, использовать локальную ссылку `self` или явно контролировать `this`:

```javascript
var self = this;

this.unsubscribe = store.subscribe(function (value)
{
    self.apply(value);
});
```

Каждый subscribe должен иметь соответствующий unsubscribe. Components с subscriptions, tasks или child components должны иметь idempotent `destroy()`:

```javascript
ControlBinding.prototype.destroy = function ()
{
    if (this.destroyed)
    {
        return;
    }

    this.destroyed = true;
    this.unsubscribers.forEach(function (unsubscribe)
    {
        unsubscribe();
    });
    this.unsubscribers = [];
};
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
var callback = this.pending[String(requestId)];
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

