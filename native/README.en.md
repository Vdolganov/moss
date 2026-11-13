# MOSS native

[Русский](README.md) · **English**

The current stable version is **0.5.2** for the original Anbernic RG35XX and Windows: four tracks, SAMPLER / MONO / ORGANISM / FM, effects, TAPE and SONG. Tested on GarlicOS 1.4.9; this build has not been retested on 1.3.9.

- [User guide](prototype/README.en.md)
- [Build and installation](../README.en.md)
- [0.5.2 hardware verification](../docs/NATIVE-VERIFICATION-052.en.md)
- [0.5.2 structure and controls](../design/current/native-v052-interaction.md)
- [Downloads](https://github.com/Vdolganov/moss/releases/tag/v0.5.2)

`prototype/` contains the shared C++17 model, UI and audio engine, plus the SDL 1.2/evdev backend for GarlicOS and the GDI/waveOut backend for Windows. `perf-baseline-v050/` is retained for comparative measurements. `tooling/` is the local compiler and SDK cache, excluded from Git; setup is described in the root README.
