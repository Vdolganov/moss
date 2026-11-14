#!/bin/sh
# Isolated maximum-load qualification; the user's projects are not used.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Ambient-Lens-v1 || exit 2
. ./cpu-performance.sh || exit 3
for unit in test-arm ambient-test-arm integration-arm; do
  ./$unit > "$unit.log" 2>&1
  status=$?; echo "$status" > "$unit.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
for mask in 17 18 24 3 10 5 9 6 12 20; do
  echo "long grains / pair $mask" > current
  ./moss --profile 11 "$mask" 30 > "pair-$mask.log" 2>&1
  status=$?; echo "$status" > "pair-$mask.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
for spec in '11 18 600' '12 18 60' '8 18 60' '6 18 60' '7 18 60'; do
  set -- $spec
  label="case-$1-$2-$3"
  echo "$label" > current
  ./moss --profile "$1" "$2" "$3" > "$label.log" 2>&1
  status=$?; echo "$status" > "$label.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
sync
echo done > complete
