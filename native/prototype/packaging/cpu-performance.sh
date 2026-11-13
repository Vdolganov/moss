#!/bin/sh
# Sourced by the app and hardware test launcher. Never overclock the RG35XX.
MOSS_CPU=/sys/devices/system/cpu/cpu0/cpufreq
MOSS_OLD_GOV=$(cat "$MOSS_CPU/scaling_governor") || exit 1
MOSS_OLD_MIN=$(cat "$MOSS_CPU/scaling_min_freq") || exit 1
MOSS_OLD_MAX=$(cat "$MOSS_CPU/scaling_max_freq") || exit 1
moss_restore_cpu() {
  echo 240000 > "$MOSS_CPU/scaling_min_freq"
  echo "$MOSS_OLD_MAX" > "$MOSS_CPU/scaling_max_freq"
  echo "$MOSS_OLD_MIN" > "$MOSS_CPU/scaling_min_freq"
  echo "$MOSS_OLD_GOV" > "$MOSS_CPU/scaling_governor"
}
trap moss_restore_cpu EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
echo 240000 > "$MOSS_CPU/scaling_min_freq" || exit 1
echo 1008000 > "$MOSS_CPU/scaling_max_freq" || exit 1
echo performance > "$MOSS_CPU/scaling_governor" || exit 1
