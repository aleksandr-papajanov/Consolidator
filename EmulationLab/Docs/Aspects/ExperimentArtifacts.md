# Артефакты экспериментов

## Локальные артефакты

`.gitignore` исключает Measurements, diagnostics, results, аудио, сборки и содержимое
EmulationLab/Experiments. В Experiments сохраняются manifest.json, README.md и
исходники/скрипты. RawAudio остаётся локальным вместе с catalog.json; его описание [RawAudio](RawAudio.md) хранится в репозитории. Результаты экспериментов не добавлять в исходный код или Docs.
Игнорирование не удаляет файлы и не снимает уже отслеживаемые файлы с учёта Git.

