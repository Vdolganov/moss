# REEL motion evidence / Результаты проверки

Summary: [English](../../REEL-MOTION-v1.en.md) · [Русский](../../REEL-MOTION-v1.md).

- `manifest.json`: exact device binaries and source hashes, comparisons, scope and remaining gates.
- `baseline-*.json` / `candidate-*.json`: alternating 30-second hardware comparisons.
- `sampler.json`, `mixed.json`, `song.json`: full-engine scenario coverage.
- `soak-reel-pond.json`: original target 1200 seconds; cleanly stopped at the user's request after **906 seconds**. The native `profile_passed:false` is retained because the original duration was not reached. All error counters are zero. This is not represented as a completed 20-minute run.
- `executed-hardware-series.sh`: the actual original script; current tooling defaults to shorter checks.
- `arm-unit.txt`, `host-unit.txt`, `host-ubsan.txt`, `host-regressions.json`: functional checks and platform equivalence.
- `audio-metrics.json`, `audio-levels.csv`, `response.png/svg`: output-tone, neutral-path, pitch and frequency-response evidence.

Исходные отчёты сохранены без исправления флагов. Длинный тест сокращён по просьбе пользователя: 906 секунд вместо 1200, а не скрыто объявлен полным. Кандидат не устанавливался поверх пользовательской версии. После испытаний Garlic и обычный режим CPU восстановлены.

Raw musical renders are reproducible with the documented renderer/analysis commands and live locally in `work/reel-motion/audio`; generated WAV/float data are intentionally outside Git. No commercial sample-pack files are included.
