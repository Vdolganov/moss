# MOSS documentation

[Русский](README.md) · **English**

The stable application version is **0.5.2**; experimental **0.6.6-dev / GRANULAR** is developed separately. Use the native guide for RG35XX and Windows. The browser prototype is a separate implementation without audio, with different bindings and limits.

| Document | Русский | English |
|---|---|---|
| Overview, installation and build | [README](../README.md) | [README](../README.en.md) |
| Native application documentation entry point | [Native](../native/README.md) | [Native](../native/README.en.md) |
| Complete 0.5.2 user guide | [Руководство](../native/prototype/README.md) | [User guide](../native/prototype/README.en.md) |
| 0.5.2 device verification | [Отчёт](NATIVE-VERIFICATION-052.md) | [Verification](NATIVE-VERIFICATION-052.en.md) |
| 0.5.1 performance: optimization history | [Отчёт](NATIVE-PERFORMANCE-051.md) | [Performance](NATIVE-PERFORMANCE-051.en.md) |
| Browser prototype v0.6 | [Руководство](<../visual prototype/README.md>) | [Guide](<../visual prototype/README.en.md>) |
| 0.5.2 release notes | [Релиз](../releases/0.5.2/RELEASE.md) | [Release notes](../releases/0.5.2/RELEASE.en.md) |
| GRANULAR / LENS: experimental 0.6.6-dev | [Инструмент](GRANULAR-INSTRUMENT.md) | [Instrument](GRANULAR-INSTRUMENT.en.md) |
| DRIVE / ORBIT: 0.6.2-dev | [Эффекты и проверка](FX-CHARACTER-v1.md) | [Effects and verification](FX-CHARACTER-v1.en.md) |
| Ambient LENS: 0.6.4-dev | [Пэды, JUMP и проверка](AMBIENT-LENS-v1.md) | [Pads, JUMP and verification](AMBIENT-LENS-v1.en.md) |
| WAV catalog: 0.6.3-dev | [Изменения и проверка](CATALOG-v063.md) | [Changes and verification](CATALOG-v063.en.md) |
| Cloud, envelopes and gain / 0.6.1 | [Проверка](GRANULAR-CLOUD-v2.md) | [Verification](GRANULAR-CLOUD-v2.en.md) |
| LOCK ONLY / REPEAT: 0.6.5-dev | [Управление](TRIGLESS-LOCKS-PLAN.md) | [Controls](TRIGLESS-LOCKS-PLAN.en.md) |
| SWING / SHIFT: 0.6.6-dev | [Управление и проверка](SWING-v1.md) | [Controls and verification](SWING-v1.en.md) |
| LOCK ONLY / REPEAT: hardware | [Проверка](TRIGLESS-REPEAT-v1.md) | [Verification](TRIGLESS-REPEAT-v1.en.md) |
| GRANULAR: integration and RG35XX verification | [Отчёт](GRANULAR-NATIVE-v1.md) | [Verification](GRANULAR-NATIVE-v1.en.md) |
| Effects: Elektron research and redesign plan | [Исследование](ELEKTRON-FX-RESEARCH.md) | [Research](ELEKTRON-FX-RESEARCH.en.md) |
| REEL: GrainBrick, RC-20, measurements and redesign | [Исследование и план](REEL-RESEARCH-PLAN.md) | [Research and plan](REEL-RESEARCH-PLAN.en.md) |
| REEL: clean path and WOW/FLUTTER audio candidate | [Реализация и проверка](REEL-MOTION-v1.md) | [Implementation and verification](REEL-MOTION-v1.en.md) |
| Elektron: firmware, memory and DSP analysis | [Разбор](ELEKTRON-FIRMWARE-ANALYSIS.md) | [Analysis](ELEKTRON-FIRMWARE-ANALYSIS.en.md) |

## Technical contracts

First experimental stage: [granular engine and test rig](GRANULAR-LAB-v1.en.md) · [Русский](GRANULAR-LAB-v1.md). The engine was subsequently integrated into 0.6.0-dev; this is not part of release 0.5.2.

These documents were originally written in English. The complete user guide covers the same user workflows in Russian:

- [0.5.2: sequences, SONG, fine/coarse editing and pages](../design/current/native-v052-interaction.md).
- [0.5.1: limits, instruments, sampler and shared interactions](../design/current/native-v051-interaction.md). The 0.5.2 contract takes precedence where they differ.

## History and working references

[PRODUCT.md](../PRODUCT.md) and [DESIGN.md](../DESIGN.md) contain the Russian product/design requirements and decision history. Older ARM plans, MOSS LAB documents, early scenarios, diagrams and review records remain in their original language as a development archive. They do not replace the current user guide; their former limits and gestures may differ from 0.5.2.

Documentation translations are maintained on `main`. The `v0.5.2` tag and original installation archives identify the released build. The English guide is provided separately in the [release](https://github.com/Vdolganov/moss/releases/tag/v0.5.2), without replacing application binaries.
