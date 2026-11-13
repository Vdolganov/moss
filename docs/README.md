# Документация MOSS

**Русский** · [English](README.en.md)

Стабильная версия приложения — **0.5.2**; отдельно развивается экспериментальная **0.6.1-dev / GRANULAR**. Выбирайте нативное руководство для RG35XX и Windows; браузерный прототип — отдельная реализация без звука с другой раскладкой и ограничениями.

| Документ | Русский | English |
|---|---|---|
| Обзор, установка и сборка | [README](../README.md) | [README](../README.en.md) |
| Нативное приложение: навигация по документации | [Native](../native/README.md) | [Native](../native/README.en.md) |
| Полное руководство 0.5.2 | [Руководство](../native/prototype/README.md) | [User guide](../native/prototype/README.en.md) |
| Проверка на устройстве 0.5.2 | [Отчёт](NATIVE-VERIFICATION-052.md) | [Verification](NATIVE-VERIFICATION-052.en.md) |
| Производительность 0.5.1: история оптимизации | [Отчёт](NATIVE-PERFORMANCE-051.md) | [Performance](NATIVE-PERFORMANCE-051.en.md) |
| Браузерный прототип v0.6 | [Руководство](<../visual prototype/README.md>) | [Guide](<../visual prototype/README.en.md>) |
| Описание релиза 0.5.2 | [Релиз](../releases/0.5.2/RELEASE.md) | [Release notes](../releases/0.5.2/RELEASE.en.md) |
| GRANULAR / LENS: экспериментальная 0.6.1-dev | [Инструмент](GRANULAR-INSTRUMENT.md) | [Instrument](GRANULAR-INSTRUMENT.en.md) |
| Cloud, envelopes and gain / 0.6.1 | [Проверка](GRANULAR-CLOUD-v2.md) | [Verification](GRANULAR-CLOUD-v2.en.md) |
| Trigless locks: proposal | [План](TRIGLESS-LOCKS-PLAN.md) | [Plan](TRIGLESS-LOCKS-PLAN.en.md) |
| GRANULAR: интеграция и проверка на RG35XX | [Отчёт](GRANULAR-NATIVE-v1.md) | [Verification](GRANULAR-NATIVE-v1.en.md) |
| Эффекты: исследование Elektron и план переработки | [Исследование](ELEKTRON-FX-RESEARCH.md) | [Research](ELEKTRON-FX-RESEARCH.en.md) |
| Elektron: разбор прошивок, памяти и DSP | [Разбор](ELEKTRON-FIRMWARE-ANALYSIS.md) | [Analysis](ELEKTRON-FIRMWARE-ANALYSIS.en.md) |

## Технические контракты

Первый этап эксперимента: [гранулярный движок и испытательный стенд](GRANULAR-LAB-v1.md) · [English](GRANULAR-LAB-v1.en.md). Затем движок интегрирован в 0.6.1-dev; это не часть релиза 0.5.2.

Эти документы изначально написаны на английском; те же пользовательские сценарии по-русски описаны в полном руководстве:

- [0.5.2: секвенции, SONG, точное/крупное изменение и страницы](../design/current/native-v052-interaction.md).
- [0.5.1: ограничения, инструменты, сэмплер и общие взаимодействия](../design/current/native-v051-interaction.md). Контракт 0.5.2 имеет приоритет при расхождениях.

## История и рабочие материалы

[PRODUCT.md](../PRODUCT.md) и [DESIGN.md](../DESIGN.md) содержат текущие русские требования и историю решений. Старые ARM-планы, MOSS LAB, ранние сценарии, диаграммы и протоколы review сохранены на исходном языке как архив разработки. Они не заменяют актуальное руководство; их прежние лимиты и жесты могут отличаться от 0.5.2.

Переводы документации обновляются на `main`. Тег `v0.5.2` и исходные установочные архивы фиксируют выпущенную сборку. Английское руководство добавлено к [релизу](https://github.com/Vdolganov/moss/releases/tag/v0.5.2) отдельным файлом, без замены бинарников.
