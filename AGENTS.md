# Experimental architecture policy

This repository is in active experimental development. Do not preserve or add
backward compatibility unless the user explicitly requests it.

Never add compatibility aliases, migration adapters, fallback paths, legacy
branches, deprecated wrappers, or duplicate APIs merely to keep old code
working. When the architecture changes, update all callers and delete the old
model directly.

If a requested change would cause an unusually destructive or externally
irreversible action, report that fact and ask for confirmation. For ordinary
code refactors, proceed directly with the clean target design.

Do not build, run tests, or perform other verification unless the user
explicitly asks for it. Do not announce an intended action and then stop
before completing the current request. Continue the requested work until its
scope is finished; do not pause for intermediate confirmation.

## Сборка

Для полной сборки проекта использовать `.vscode/build-all.cmd` из корня
репозитория. Скрипт сам настраивает CMake и собирает конфигурацию
`RelWithDebInfo`. Не использовать альтернативные команды сборки без явного
запроса пользователя.

## Перед коммитом

Перед созданием коммита проверять затронутую документацию. Если изменение
затрагивает архитектуру, правила, структуру проекта или workflow, обновлять
соответствующую документацию в том же изменении.

Перед коммитом оценивать необходимость тестов для изменённого поведения. Если
тесты требуются, добавлять или обновлять их по схеме из
`Consolidator.Native/Tests/README.md`: отдельно component, command и integration
уровни в зависимости от затронутых границ. Затем выполнить разрешённую полную
сборку и подходящие тесты и убедиться, что изменение работает. Не добавлять
формальные тесты для изменений, которые не меняют проверяемое поведение.
