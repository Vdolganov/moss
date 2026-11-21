# Slicer 0.11.1 verification · 18 November 2026

[Controls / управление](../../SAMPLE-FOLDERS.md) · [English](../../SAMPLE-FOLDERS.en.md)

The complete existing desktop suite plus the expanded sample-folder and new audio-level tests passed. Both new scoped tests also passed on the original RG35XX / GarlicOS 1.4.9. Tests cover physical bank changes/held triggers, exact fine bounds, non-integer-kHz and large frame positions, contiguous 1 MiB defaults, arrangement rejection without mutation, exact stereo min/max including all source frames, kit export/reload and previous folder/cache/persistence behavior.

Baseline: source audition RMS 8100, prepared audition 6377 (−2.0774 dB); MASTER recording was −5.1055 dB relative to monitored output. After: both audition RMS values 12000; master capture matches output with zero PCM difference. PCM16 cuts without fades match their source samples exactly. These are digital level/integrity checks, not a subjective headphone listening test. Sample-track velocity, key/track/master levels remain in the musical signal path.

## Hardware

Three production-binary profiles, 10 seconds each / 942 callbacks total, four tracks and full 32 MiB PCM cache. Sampler + ORBIT/REEL: maximum 25.434 ms; granular + POND/REEL: 29.577 ms; ORGANISM + ORBIT/REEL: 27.291 ms, against a 32 ms callback budget. All three: zero over-budget callbacks, late intervals, stream underruns, scheduling overflows or realtime-priority errors. These are bounded short stress checks, not a claim of indefinite stability or exhaustive effect combinations.

Real evdev walkthrough in an isolated test installation: open WAV → split four → move part 2 → L1/R1 TRIM/FINE → two single-frame start edits → zoom → audition → ARRANGE PARTS IN ORDER → name/save folder → Y assign four → MENU exit. The saved hardware WAVs were read back and matched the original PCM with lengths 32000 / 31998 / 32000 / 32000 frames, demonstrating preserved two-frame precision after rearranging and saving. The isolated interactive report is separate from the strict profiles: 2813 callbacks, zero over-budget callbacks/stream underruns, and five late intervals. It includes filesystem operations that pause audio; the exact cause of those five intervals was not isolated, so this UI run is not claimed as a clean stress pass.

## Deployment

MOSS 0.11.1 and its launcher installed, read-back hash matches the stress-tested binary. Previous binary/launcher saved under `/mnt/mmc/MOSS-Backups/20261118-slicer-v0111`. Fresh project/library backup on the PC; all 75 user files have identical SHA-256 before and after normal installed launch/exit. Garlic menu restored, filesystem synced, governor restored to interactive. GitHub publication was outside this task.

## Русский

Полный набор desktop-проверок прошёл; проверки нарезки/папок и уровня звука выполнены также на ARM. На Anbernic три нагрузочных прогона по 10 секунд прошли без пропусков аудио. Максимальный callback — 29,577 мс при бюджете 32 мс. Увеличения лимита PCM нет.

Кнопками устройства пройден новый редактор, сохранение и назначение четырёх частей. После двух точных сдвигов сохранённый участок имеет 31998 фреймов, а PCM всех четырёх WAV точно совпал с исходником. Это подтверждение цифрового уровня и сохранности; субъективное прослушивание в наушниках остаётся за пользователем.

Установлена проверенная 0.11.1. Все 75 пользовательских файлов сохранены без изменений; старая сборка и данные скопированы. Консоль возвращена в Garlic, запись завершена.
