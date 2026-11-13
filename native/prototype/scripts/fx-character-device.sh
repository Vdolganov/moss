#!/bin/sh
# Isolated qualification: never run in the user's project/library directory.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-FX-Character-v1 || exit 2
. ./cpu-performance.sh || exit 3
./test-arm > unit.log 2>&1
status=$?; echo "$status" > unit.exit
[ "$status" -eq 0 ] || exit "$status"
# All ten legal pairs, four GRANULAR tracks, maximum timeline and PCM bank.
for mask in 17 18 24 3 10 5 9 6 12 20; do
  ./moss --profile 5 "$mask" 30 > "pair-$mask.log" 2>&1
  status=$?; echo "$status" > "pair-$mask.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
# CURVE crossfades, short grains, changing envelopes/FREEZE, mixed instruments,
# all 16 sampler slots, each synth, and SONG transitions through all FX pairs.
for spec in '10 18 600' '10 17 60' '8 18 60' '9 18 60' '6 18 60' '0 17 60' '1 18 30' '2 18 30' '3 18 30' '7 18 60'; do
  set -- $spec
  label="case-$1-$2-$3"
  ./moss --profile "$1" "$2" "$3" > "$label.log" 2>&1
  status=$?; echo "$status" > "$label.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
sync
echo done > complete
