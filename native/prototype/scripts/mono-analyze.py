"""Measure dry mono-render output and create 16-bit audition WAVs (requires numpy)."""
from pathlib import Path
import json
import sys
import wave
import numpy as np

directory = Path(sys.argv[1] if len(sys.argv) > 1 else "work/mono-research/audio")
names = ["signal", "soft", "acid", "sub"]
results = []
for index, name in enumerate(names):
    samples = np.fromfile(directory / f"mono-{index}.f32", dtype="<f4")
    assert len(samples) == 32000 * 12 and np.all(np.isfinite(samples))
    peak = float(np.max(np.abs(samples)))
    assert peak <= .981
    with wave.open(str(directory / f"mono-{name}.wav"), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(32000)
        output.writeframes(np.rint(samples * 32767).astype("<i2").tobytes())
    results.append({"preset": name, "rate": 32000, "seconds": 12,
                    "peak": peak, "rms_dbfs": float(20 * np.log10(np.sqrt(np.mean(samples * samples)))),
                    "mean": float(np.mean(samples)), "nonfinite": 0,
                    "samples_at_limit": int(np.sum(np.abs(samples) >= .98))})
(directory / "analysis.json").write_text(json.dumps(results, indent=2) + "\n")
print(json.dumps(results, indent=2))
