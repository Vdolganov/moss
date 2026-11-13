# MOSS native

Текущая стабильная версия — **0.5.2** для оригинальной Anbernic RG35XX и Windows. Четыре дорожки, SAMPLER / MONO / ORGANISM / FM, эффекты, TAPE и SONG. Проверена на GarlicOS 1.4.9; эта сборка на 1.3.9 ещё не перепроверялась.

- [Руководство приложения](prototype/README.md)
- [Сборка и установка](../README.md)
- [Аппаратная проверка 0.5.2](../docs/NATIVE-VERIFICATION-052.md)
- [Структура и управление 0.5.2](../design/current/native-v052-interaction.md)
- [Готовые сборки](https://github.com/Vdolganov/moss/releases/tag/v0.5.2)

`prototype/` содержит общие C++17 модель, UI и аудиодвижок, а также SDL 1.2/evdev backend для GarlicOS и GDI/waveOut backend для Windows. `perf-baseline-v050/` сохранён для сравнительных измерений. `tooling/` — исключённый из Git локальный компилятор и SDK; подготовка описана в корневом README.
