#!/bin/sh
# Standalone experiment. Run inside Garlic's /cfw chroot; no project access.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH
cd /mnt/mmc/MOSS-Granular-Lab || exit 2
. ./cpu-performance.sh || exit 3
SECONDS_PER_CASE=${1:-3}
uname -a > hardware.txt
cat /proc/meminfo >> hardware.txt
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq >> hardware.txt
: > headless.jsonl
FAILED=0
for capacity in 8 16 32; do
  for tracks in 1 4; do
    ./benchmark-arm "$tracks" "$capacity" 0 0 "$SECONDS_PER_CASE" >> headless.jsonl || FAILED=1
  done
  for effects in 3 5 9 6 10 12; do
    ./benchmark-arm 4 "$capacity" "$effects" 0 "$SECONDS_PER_CASE" >> headless.jsonl || FAILED=1
  done
  for mode in 1 2; do
    ./benchmark-arm 4 "$capacity" 10 "$mode" "$SECONDS_PER_CASE" >> headless.jsonl || FAILED=1
  done
done
for capacity in 8 12 16; do
  for effects in 3 5 9 6 10 12; do
    ./benchmark-arm 4 "$capacity" "$effects" 3 "$SECONDS_PER_CASE" >> headless.jsonl || FAILED=1
  done
done
cat headless.jsonl
sync
exit "$FAILED"
