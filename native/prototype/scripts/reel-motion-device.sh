#!/bin/sh
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Reel-Motion-v5 || exit 2
. ./cpu-performance.sh || exit 3
./test-arm > unit.log 2>&1
status=$?; echo "$status" > unit.exit
[ "$status" -eq 0 ] || exit "$status"
run_profile() {
  name=$1; binary=$2; kind=$3; mask=$4; seconds=$5
  echo "$name" > current
  ./$binary --profile "$kind" "$mask" "$seconds" > "$name.log" 2>&1
  status=$?; echo "$status" > "$name.exit"
  report=$(ls -t perf/run/reports/*.json | head -n 1)
  cp "$report" "$name.json" || exit 4
  [ "$status" -eq 0 ] || exit "$status"
}
if [ -f quick ]; then
  run_profile baseline-10 moss-baseline 17 10 20
  run_profile candidate-10 moss 17 10 120
  run_profile mixed moss 19 24 20
  sync
  echo done > complete
  exit 0
fi
# Interleave old and new on identical engines, assets, UI and hardware.
for mask in 9 10 12 24 18; do
  run_profile "baseline-$mask" moss-baseline 17 "$mask" 30
  run_profile "candidate-$mask" moss 17 "$mask" 30
done
run_profile sampler moss 18 10 30
run_profile mixed moss 19 24 30
run_profile song moss 20 10 60
sync
echo done > matrix-complete
run_profile soak-reel-pond moss 17 10 180
sync
echo done > complete
