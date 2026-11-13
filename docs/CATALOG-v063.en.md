# Source WAV catalog · 0.6.3-dev

[Русский](CATALOG-v063.md) · **English**

The 128-file limit hid part of larger sample packs. The catalog now holds **2048 source WAV files**, and R1+←/→ moves the selection by seven rows, matching one visible page. Recursive subfolder discovery still produces one flat list, without folder navigation. Search depth is unchanged: the root and five levels below it.

The path table uses an additional **960 KiB**. Scanning does not load audio data: the prepared bank remains limited to 32 assets, each up to 1 MiB PCM16. The decoder, streaming player, DSP and MOS9 format are unchanged. The stable MOSS 0.5.2 installation was not updated.

## Verification on November 13, 2026

- Windows and the original RG35XX / GarlicOS 1.4.9 opened all **469 SHAPES WAV headers**. Tests covered crossing index 128, a partial last page, the 2048-entry bound and adding a source to a full catalog.
- An isolated test directory was used to verify streaming preview of a large WAV and saving a named slice within 1 MiB. Test saves did not use the user's prepared library.
- The normal application displayed **570 files**: 567 shared library files and three captures. UI checks covered selecting file 141, previewing it and opening EDIT with its 1024 KiB bound.
- MENU exited with code 0 and restored the `interactive` governor. All nine sequence/SONG and metadata files matched their backup SHA256 hashes.

A broader probe of the existing library stopped at `Master Sample Pack/Hollywoodedge,_Screams_2_Woman_Singl_PE133601.wav`: the current decoder rejected its format (`USE PCM16/24/32 OR FLOAT32 WAV`). Successful format verification therefore covers all 469 new WAV files, not all 567 existing and new files. Format support was not changed in this update.

Long audio stress tests were **not repeated**, as requested. The normal session recorded no audio processing overruns or stream underruns, but did record three long callback intervals and three slow UI frames, peaking at 60.517 ms. This verifies the catalog and interaction, not maximum DSP load. The 0.6.2 stress report remains tied to its own exact binary.

Evidence and the new binary SHA256 are in [manifest.json](verification/catalog-v063/manifest.json), alongside targeted test and normal launch logs. User audio files are not included in the repository.

Targeted test source: `native/prototype/tests/catalog_test.cpp`. Arguments: WAV pack root and expected file count. Run from an isolated empty working directory: the test saves one prepared slice into its `library/` folder.
