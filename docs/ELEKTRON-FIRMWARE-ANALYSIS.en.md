# Elektron firmware architecture, DSP and implications for MOSS

[Русский](ELEKTRON-FIRMWARE-ANALYSIS.md) · **English** · 13 November 2026

Four official firmware images were unpacked; their MAIN OS images were disassembled and selected functions partially decompiled in Ghidra. Digitakt II's separate SHARC boot stream was parsed and 19 audio-code windows inspected. This is instruction/data analysis, beyond parameter-name searches. Complete effect source code has not been recovered, and this research does not yet add new effects to MOSS.

This follows the [initial research and sound-design plan](ELEKTRON-FX-RESEARCH.en.md). The [manifest](verification/elektron-firmware-analysis/manifest.json) records versions, hashes, container contents, tool results and boot records.

## 1. Evidence levels

- **Verified:** observed in the downloaded image and checked against instructions, constants or data structures. For SHARC, this means an external decoder plus manual analysis, without Elektron hardware validation.
- **Source:** a finding reported by an open research project's author or an official manual. Reading it does not make it our own measurement.
- **Inference:** an interpretation or a proposal for MOSS, rather than proof of a stock effect's implementation.
- **Open:** insufficient evidence, particularly for the complete reverb network, Warble, all parameter mappings and execution cost on Elektron.

No image was executed or flashed on Elektron hardware. Neither bit-identical sound nor transferable ColdFire/SHARC performance is established.

## 2. Images and analysis performed

| Device | Official image | MAIN OS bytes | Work performed |
|---|---|---:|---|
| Digitakt MKI | [1.54](https://www.elektron.se/wp-content/uploads/2026/10/Digitakt_OS1.54_dist.zip) | 2,479,680 | ColdFire startup, shared system code and selected audio-path functions; 16 exported partial decompilations |
| Digitakt II | [1.17](https://www.elektron.se/wp-content/uploads/2026/10/Digitakt_II_OS1.17_dist.zip) | 3,275,616 | ColdFire startup/allocator, 7 partial decompilations; 19 separate SHARC DSP windows |
| Analog Heat +FX | [1.02](https://www.elektron.se/wp-content/uploads/2024/10/Analog-HeatFX_OS1.02_dist.zip) | 2,174,964 | ColdFire startup, buffers, DMA, FX dispatcher and DSP candidates; 44 partial decompilations |
| Analog Heat MKII | [1.22](https://www.elektron.se/wp-content/uploads/2024/10/Analog-Heat_MKII_OS1.22_dist.zip) | 1,234,756 | Initial ColdFire program map and 2 partial decompilations of system code |

[elektron-firmware-tool](https://github.com/mischa85/elektron-firmware-tool/tree/a5bce9a6af644386d900924082993a805e812874) parsed the containers and passed their internal checksum checks. Sections include MAIN OS, bootstrap, updater and metadata; Heat also contains FPGA data, DT2 a separate DSP payload. DT1/DT2's unknown section 8 has the same SHA256; its purpose is not established here.

Ghidra 12.2 used `68000:BE:32:Coldfire`, big-endian, at base `0x40000400`. The first word contains the entry address rather than the first ordinary instruction. Analysis started from the entry, branches and candidate function prologues. Thousands of automatically identified boundaries **are not thousands of verified functions**.

Ghidra reports unsupported instructions and truncated control flow, particularly around ColdFire EMAC. Its C-like output is not compilable recovered source. A large Heat +FX UI constructor also exceeded the decompilation time limit. DSP conclusions come from selected inspected regions, not an assumption that automatic analysis recovered everything.

### Digitakt II's separate DSP

Unpacked section 7 contains 321,016 bytes, SHA256 `0f514a12a2255f5c081e292c47f1f29462003177658da4bbae0a22fd737fffa2`. The pinned [elekloader](https://github.com/elekloader/elekloader/tree/59acfffdd9bb8415e3049d1977ff2a6fae03bb0e) parser validated 104 ADI boot records, including LOAD/FILL and FIRST/INIT/FINAL metadata. Records must be applied in order; later records can overwrite earlier ones.

This is not a flat ColdFire program. It loads code/data into multiple SHARC regions. LOAD #69 places 109,436 bytes at `0x20000000`; #93 places 104,500 bytes at `0x283827cc`. Instruction PC16 addresses differ from loader byte addresses: the inspected L1 region maps through `0x28000000 + 2 × PC16`; the selected L2 window through `0x20000000 + 2 × (PC16 − 0xb80000)`.

The pinned [digikit decoder](https://github.com/ElekLabs/digikit/tree/5f6be8326147fe41cf06bb19358ae2916b2f3f63) handles 16/32/48-bit VISA instructions. Each selected window reached its first linear RFRAME epilogue without an unknown format. This does **not** validate the complete control-flow graph or every instruction's semantics. No SHARC C decompilation was performed.

## 3. Shared architecture and model differences

| Model | Control and audio | Research implication |
|---|---|---|
| DT1 | ColdFire MAIN OS; DT-FM documents stock 32-frame / 48 kHz audio blocks and integer buffers | Study EMAC, hot-code placement and bounded blocks; do not assign Digitone's two-CPU architecture to it |
| DT2 | ColdFire MAIN OS plus separate SHARC DSP, supported by the images and loader format | UI/file memory and DSP memory are distinct. MAIN OS does not contain all audio processing |
| Heat +FX | ColdFire, FPGA payload, analog HEAT and digital effects | Useful for buffering, routing and digital FX; analyze analog saturation separately |
| Heat MKII | ColdFire/FPGA plus physical analog circuits and their control | Firmware does not contain eight ready-made digital emulations of its distortion circuits |

**Verified:** all four MAIN OS images contain similar context-switch code at `0x40000410`: saving D/A registers, changing a state pointer, handling interrupt registers and returning with RTE. C++ type names, virtual tables and runtime traces are visible. This supports a common embedded platform with control tasks, specialized audio paths and direct peripheral access. The RTOS identity and complete task-priority map remain unknown.

**Source:** elekloader's DT2 profile identifies ADSP-21569. [Analog Devices](https://www.analog.com/en/products/adsp-21569.html) documents SHARC+ SIMD, internal SRAM and extensive DMA support. These features help explain why clock speeds and instruction counts cannot be compared directly with RG35XX. A datasheet maximum clock is not a measurement of this firmware's operating frequency.

**Source:** the [MCF5441x reference manual](https://www.nxp.com/docs/en/reference-manual/MCF54418RM.pdf) specifies separate 8 KiB instruction/data caches, 64 KiB SRAM and EMAC. Small caches make placement important. A mod's `-mcpu=54455` compiler flag alone does not identify the physical chip marking.

## 4. Memory and data movement: concrete findings

### DT2 system memory and buddy allocator

**Verified in OS 1.17:** startup copies `0x40312000..0x40318e80` and `0x40318e80..0x4031ff60` into SRAM at `0x80000000` and `0x80008000`, then clears the remaining space in each half. These are ColdFire addresses, not SHARC addresses.

Allocation is at `0x4011e954`, free at `0x4011eb5a`, buddy merging at `0x4011e7bc`. Metadata at `0x402b45ac` defines `0x44f39c40..0x46f39c40`: a **32 MiB** pool, minimum block 16 bytes, maximum order 21. Power-of-two rounding, splitting/merging, free-block structures and a lock at `0x44e08654` are visible.

Addresses from external OS 1.16 research did not match 1.17; the functions were located and checked again. This is why hashes and versions matter.

**Inference:** Elektron has dynamic allocation in its control-side software. That does not demonstrate `malloc` in the audio path. MOSS should prepare FX states and large resources outside the callback; there is no reason to copy the entire allocator.

### Heat +FX: long lines and small working blocks

**Verified:** initializer `0x40015b0c` clears **4 MiB** at `0x46c00000`, sets working state in `0x8000....` and configures eDMA records including `0xfc0453c4...`. Initializer `0x4001602e` prepares `0x8000` bytes at `0x468a0000`, `0x90000` bytes at `0x46810000`, linked descriptors and multiple state banks.

Dispatcher `0x40017a94` selects FX branches using fixed 256-byte scratch blocks. Separate processing wrappers and memory initializers are visible. Some effect identities are inferred from calls and UI parameter layout rather than fully proven.

**Inference:** long delay storage and frequently accessed working state have different memory requirements. Block transfers and locality deserve attention alongside arithmetic cost. The `0x46...` addresses are not asserted to be uncached aliases without decoding the relevant controller configuration.

### DT2 PCM ownership and cache visibility

**Source:** digikit's [sample-cache visibility](https://github.com/ElekLabs/digikit/blob/5f6be8326147fe41cf06bb19358ae2916b2f3f63/docs/findings/108-dt2-sample-cache-visibility.md) and [sample-pool](https://github.com/ElekLabs/digikit/blob/5f6be8326147fe41cf06bb19358ae2916b2f3f63/docs/findings/109-dt2-stock-pool-borrowing.md) findings describe DMA into staging, subsequent CPU copying of PCM and stable PCM offsets when metadata moves. We did not validate this on hardware; the former document also states its own hardware-validation limit.

**For MOSS:** the loader owns an incomplete resource, audio receives a completed immutable buffer, and old storage survives until no voice uses it. Active grains must not read relocated/overwritten PCM. Deduplication, generation/reference tracking and resource management belong outside the sample-processing loop.

## 5. Overdrive: a concrete nonlinear section recovered

**Verified:** DT2 OS 1.17's handler at PC16 `0x1cb3d8` clips the driven signal to ±1 and, on the inspected path, applies three iterations of:

`f(z) = z × (2 − |z|)`

The instructions contain **1.0** and **2.0**, ABS, subtraction and multiplication. One external description loosely calls this “1 − |x| style”; the constant in this section is 2.

For positive input in [0, 1], the triple composition is `1 − (1 − z)^8`, with an odd negative branch. This is a mathematical consequence of the local curve. The inspected path also shows `dry + mix × (shaped − dry)` and per-sample coefficient increments within a block.

**Useful principle:** an expressive curve can use multiplication and ABS instead of an expensive `tanh`. Input scaling, drive range, mixing and coefficient motion still determine the sound.

**Open:** full UI-to-coefficient mapping, every mode and surrounding stage, DC handling, and the location/existence of oversampling. This is not the Analog Heat algorithm or a complete account of Elektron master distortion.

**For MOSS:** a separate DRIVE with original curve variants, pre/post filtering, DC blocking, independent DRIVE/OUTPUT and level-matched comparisons. Check aliasing at our 32 kHz rate. Keep saturation separate from bit/rate reduction and coefficient preparation separate from sample processing.

## 6. Delay: a chain of stages

**Verified:** the DT2 wrapper at `0xb82150` calls `0xb81f28`, `0xb81baf`, `0xb819bd`, two instances of filter `0x1ccbd8`, and two additional finishing handlers. The first region prepares parameters; it contains constants 48,000 and 384,000 and smoothed state updates. Exact field meanings and every finishing stage remain unmapped.

**Source:** [digikit's DSP map](https://github.com/ElekLabs/digikit/blob/5f6be8326147fe41cf06bb19358ae2916b2f3f63/docs/findings/53-stock-dsp-shape.md) reports 524,288-float rings and an eight-second time limit. Our constants are consistent with this, but the ring size was not independently recovered in this pass. The document's static instruction counts are not device execution times.

A separate primitive at `0x1cbf07` uses a 13-bit ring mask and multi-point reading with tabulated coefficients. It is an interpolation reference, **not** evidence that the shared delay has only an 8192-sample ring.

**For ORBIT:** independent repeat HPF/LPF, feedback, stereo routing and musical TIME transitions matter more than simply enlarging a buffer. Pitch glide and crossfading two read positions are different behaviors; interpolation alone does not remove the pitch change from moving a read head. Tails must continue through steps without a new note.

## 7. Reverb: stages verified, exact network still open

**Verified:** DT2 wrapper `0xb80fe0` calls these stages in order:

| PC16 address | Observation | Interpretation limit |
|---|---|---|
| `0xb80e40` | Prepares state/coefficient values | Parameter-update candidate |
| `0xb80450` | Buffer processing before the main stage | Predelay/read candidate |
| `0x1ccbd8` | Filter block with state writeback | Biquad primitive; exact direct form not established here |
| `0xb8037f` | Additional pre-tank processing | Role needs further tracing |
| `0xb8063e` | Large internal network | Main tank candidate |
| `0xb80cdf` | Final processing | Output-stage candidate |

Inside `0xb8063e`, we checked **eight calls** to `0xb8033b`, **two** to `0xb8055d`, and a call to `0xb80314`. The first primitive has stateful recursion. The second uses fractional read positions, four coefficient-table groups spaced by `0x80`, and ring reads. The third updates two persistent quantities using the block size.

This provides concrete leads for filtering, interpolation and tail motion. Eight filter calls do **not** prove an eight-line FDN; four coefficients do not establish a particular cubic polynomial. The full feedback matrix, delay lengths, coefficients and parameter meanings remain unrecovered. Calling the stock reverb Dattorro, Freeverb or a plate would currently be speculation.

**For POND:** compare original compact modulated-network and FDN candidates for density, metallic resonances, stereo behavior and frequency-dependent decay. Independent predelay, diffusion, damping and motion offer more control than a single long DECAY. Do not round musical delay lengths to powers of two just to obtain cheap wrap masks: storage capacity and active delay length can differ.

## 8. Analog Heat and tape/Warble

The [Heat MKII manual](https://www.elektron.se/wp-content/uploads/2024/09/Analog-Heat-MKII-User-Manual_ENG_OS1.21C_220202.pdf) describes physical analog distortion circuits, filtering and EQ. Decompiling control software cannot recover their electrical transfer characteristics. Accurate modeling would require circuit information or measurements of real hardware across levels and frequencies.

For digital tape behavior, **Heat +FX / Warble** is more relevant. Its pages, controls and processing candidates are present in the image, but the mathematical kernel has not been fully traced. The [+FX manual](https://www.elektron.se/wp-content/uploads/2024/09/Analog_Heat_FX_User_Manual_ENG_OS1.01_240325.pdf) separates instability, stereo and noise controls. It documents operation, not a tape-hysteresis equation.

**For REEL:** separate recording saturation, coloration, slow wow, fast flutter and noise. Add irregular motion, controllable losses and aligned dry/wet behavior. Warble is a reference for expression and control, not a basis for claiming an exact tape-physics model or a Heat clone.

## 9. Open projects worth following

The manifest pins six primary research repositories. Additional projects below were reviewed through their public descriptions and have not passed our DSP audit.

| Project | Value to MOSS | Boundary |
|---|---|---|
| [DT-FM](https://github.com/mtalikka/DT-FM/tree/a8ea4d162946321d0ea59ed39a77f04b96512fc0) | Compact four-operator FM, static voices, slower control updates, LUTs | 24→48 kHz processing and a 65-point soft clip belong to the **mod**, not stock Elektron. MIT |
| [RingTone / Tone+FX](https://github.com/DigiAlchemydsp/RingTone/tree/28298f8ecbc4937852ba2dd6361cea8f913a15af) | Digitone MKI research, fixed-point FX, smaller networks and memory budgets | Its author documents Digitone's two-CPU design; we did not decompile an official Digitone image here. GPL-2.0 |
| [elekloader](https://github.com/elekloader/elekloader/tree/59acfffdd9bb8415e3049d1977ff2a6fae03bb0e) | Formats, version profiles, address validation, SHARC boot parsing | Research/patching infrastructure, not a portable effects library. GPL-2.0 |
| [digikit](https://github.com/ElekLabs/digikit/tree/5f6be8326147fe41cf06bb19358ae2916b2f3f63) | SHARC decoder, DSP maps, cache/sample ownership, hardware probes | AGPL-3.0; check per-file terms. Emulator metrics are not hardware metrics |
| [digitakt-ii-research](https://github.com/karigardarsson/digitakt-ii-research/tree/bad9a6414ed38bf962fb58f368dfdfbba033cb57) | System memory, sample editing, machine integration | Notes cover different OS versions. Root MIT does not override derived AGPL components |
| [Digiwerk / dn2-cfw](https://github.com/naturarum/dn2-cfw) | Three-head tape echo, flanger and FX routing | Original effects by its author. The reviewed release page withdraws spring reverb after hardware audio stoppages; the entire set is not a validated reference |
| [digitables](https://github.com/irpina/digitables) | Pitch/arpeggio tables integrated with locks | A Digitone modulation feature, not a wavetable synth or new DSP engine. GPL-2.0 |

For a future synth, a small independent open FM implementation is a better first candidate than transplanting Digitone. Model:Cycles and newer digital machines are possible later research subjects; their DSP was not recovered here. Analog Four/Rytm likewise require separating digital effects from physical analog voices.

Open tools and stock firmware are distinct materials. No Elektron instructions, decompiled functions or dependencies from these projects have been added to MOSS. Direct use of open implementations requires preserving their terms and attribution; the observed principles can inform original ARM code.

## 10. What transfers to MOSS

| Technique | Practical application | Avoid assuming |
|---|---|---|
| Control/audio separation | Prepare coefficients on change or at a block boundary, then ramp in DSP | File I/O, allocation and UI work belong outside the callback |
| Small working sets | Nearby coefficients/state, predictable passes, separate long rings | Elektron hardware addresses/DMA registers do not transfer |
| Smoothing | Independent gain/wet/feedback ramps and stable filter changes | Arbitrary coefficient interpolation is not necessarily stable |
| Shared spatial FX | One expensive reverb/delay fed by sends | This changes musical behavior and must not be introduced silently |
| Specialized arithmetic | Benchmark LUTs, polynomials, float32/NEON and fixed point on ARM | A ColdFire fixed-point advantage does not prove an ARM advantage |
| Bounded resources | Explicit voice, line, interpolation-read and memory limits | Do not silently shed effects under load |
| Hardware measurement | Extreme controls, cold data, UI, capture and transitions | SHARC instruction counts cannot be converted to RG35XX milliseconds |

[RingTone's optimization report](https://github.com/DigiAlchemydsp/RingTone/blob/28298f8ecbc4937852ba2dd6361cea8f913a15af/docs/OPTIMIZATION-ROUND.md) records an instructive author measurement: a fused pass cost approximately 15% more on hardware. Fewer source-level loops can increase register pressure and memory traffic. This is an external result, not our benchmark.

## 11. Sound redesign plan after research

The latest [maximum-load GRANULAR test](GRANULAR-CLOUD-v2.en.md) has a **32 ms** block budget and a worst observed block of **30.211 ms**. The remaining **1.789 ms** is small. Four tracks with two inserts each cannot be treated as unlimited capacity for a more expensive reverb.

1. **Measurement baseline.** Capture old-effect renders and level-matched comparisons. Separate internal headroom, artistic saturation and output limiting. Measure each candidate while sounding, not only bypassed.
2. **DRIVE and ORBIT.** Build original ARM candidates with curve/tone/output control and filtered feedback/stereo delay motion. Exercise large TIME/DRIVE locks, high-frequency input, DC and transitions.
3. **POND.** Compare two original tank candidates for tail quality and CPU. Measure both independent inserts and shared POND/ORBIT sends. Decide routing before changing the UI; the current two-FX limit remains in place for now.
4. **REEL.** Build separate wow/flutter/noise/coloration mechanisms, then tune their interaction. Verify neutral settings, dry/wet phase behavior and predictable level.
5. **Integration.** Two banks of four parameters, Y/B/A/X order, L1/R1 navigation, stable lock IDs, tails and SONG state transitions. Do not synchronously clear large rings in the audio callback.
6. **RG35XX acceptance.** Four tracks, every permitted effect, maximum voices/patterns, demanding settings, resampling and active UI. The provisional target from the sound plan remains p99 ≤24 ms and max ≤28 ms in comparable runs, with no overruns. These are not achieved new-FX results or guarantees for arbitrary future features.

Exact Elektron reconstruction still requires complete CFG/numerical tests, UI→DSP tables, reverb network/lengths/matrix, the Warble kernel, and audio comparison on real Elektron hardware. Those tasks need not prevent original expressive MOSS effect prototypes from starting.

## 12. Research artifacts

Git contains this bilingual report and the [manifest](verification/elektron-firmware-analysis/manifest.json). Private local material is under ignored `work/elektron-fx-research/`:

- `digitakt`, `digitakt-ii`, `heat-fx`, `heat-mkii`: official ZIP/SysEx files, extracted sections, `analysis/functions.tsv`, `instructions.tsv`, `decompilation.txt`;
- `digitakt-ii/analysis/sharc`: boot records, selected disassembly windows and `windows.json`;
- `ghidra-projects`, `tooling`: Ghidra projects and private tooling installations;
- `InspectFirmware.java`, `run-ghidra.ps1`, `prepare_sharc.py`, `inspect_sharc.py`, `research_manifest.py`: local analysis reproduction;
- pinned research repositories and `reference-notes`: external sources. The digikit checkout is incomplete on Windows because of an incompatible filename; required decoder/notes files were fetched separately at the pinned commit.

Firmware, decompiled code and stock instruction listings are not included in MOSS Git. The manifest hashes local outputs so future work can be compared with this analysis pass.
