---
name: consolidator-dsp-emulation
description: Investigate and fit Consolidator DSP to a reference with explicitly requested EmulationLab or Plugin Doctor research. Choose discriminating experiments, identify dependencies and simplify the accepted model.
---

# Исследование референса

Применять по запросу исследовать, сравнить, подобрать или уточнить модель.

## Подготовить опыт

Прочитать [AGENTS.md](../../../AGENTS.md) и документы затронутых scope по его маршруту.
Для исследования нужны [EmulationLab Rules](../../../EmulationLab/Docs/Rules.md),
[Usage](../../../EmulationLab/Docs/Aspects/Usage.md),
[MeasurementMethods](../../../EmulationLab/Docs/Aspects/MeasurementMethods.md),
актуальный Native algorithm и Findings. Для adapter читать ModelPluginsVst3 из
Architecture EmulationLab. Команды и schema брать из текущих Docs.

Установить цель: точное приближение референса, удобная модель или упрощение принятой.
Уточнить недостающее в запросе/manifest/readback; при изменении цели пересмотреть
критерий результата. Зафиксировать условия по MeasurementMethods, проверить setters
и readback до большого аудиогрида. Снять информативный нейтральный baseline.

## Выбрать следующий опыт

Начать с небольшого опыта, различающего причины оставшегося расхождения:

| Наблюдение | Полезное направление |
| --- | --- |
| Расходится плато | Readback, несколько уровней, static curve, makeup |
| Плато близко, переход нет | Step histories, attack/recovery, порядок detector update |
| Первый цикл совпадает, следующие нет | Повторы, длинный хвост, слабый сигнал после сильного |
| Ошибка зависит от уровня/частоты | Quiet baseline и обработанный сигнал на тех же частотах |
| Гармоники меняются с timing | Разделить gain modulation и дополнительную нелинейность |
| Звук уже принят | Сравнить упрощение с принятой моделью отдельно от ошибки референса |

За деталями выбора сигналов, fitting и упрощения обращаться к
[методам](references/methods.md), за полезной аналогией — к
[урокам устройств](references/device-lessons.md). Числа прошлых опытов не являются
универсальными допусками или коэффициентами.

Разделять level bias, shape/dynamics и harmonics; не выбирать по score, скрывающему
ухудшение отдельных сигналов. Сначала сформулировать зависимость, затем подбирать
небольшое число коэффициентов с bounds. Следующим опытом проверить перенос на
другой уровень/историю/настройку, когда это различает модели. Fit сам по себе не
доказывает внутреннюю схему устройства.

## Принять и закончить

Перенести принятый закон в общую Native-модель по [Native Rules](../../../Consolidator.Native/Docs/Rules.md).
При упрощении сравнивать с принятой версией, отделяя его ошибку от ошибки референса.
Исправлять обнаруженные причины в algorithm/host/readback/measurement infrastructure,
а не компенсировать дефект fitting-коэффициентами.

Закрепить принятый algorithm/control law в Native aspect, наблюдения с условиями
и границами — в Findings, использование — в manifest опыта. Артефакты обработать
по [ExperimentArtifacts](../../../EmulationLab/Docs/Aspects/ExperimentArtifacts.md).
Сообщить зависимости, сигналы/настройки, исправления, результат и пределы обобщения;
отделить измерение от гипотезы и source review. Разрешения и ограничения исследования
определяются AGENTS/Rules, а не этим skill.
