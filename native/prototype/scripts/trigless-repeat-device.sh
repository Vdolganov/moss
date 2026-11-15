#!/bin/sh
# Isolated load qualification. Does not open user projects or sample libraries.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Trigless-Repeat-v1 || exit 2
. ./cpu-performance.sh || exit 3
./trigless-repeat-arm > unit.log 2>&1
status=$?; echo "$status" > unit.exit
[ "$status" -eq 0 ] || exit "$status"
for mask in 17 18 24 3 10 5 9 6 12 20; do
  echo "held notes, lock-only and repeat / pair $mask" > current
  ./moss --profile 13 "$mask" 30 > "pair-$mask.log" 2>&1
  status=$?; echo "$status" > "pair-$mask.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
for spec in '13 18 300' '14 18 120' '15 18 120' '16 18 60'; do
  set -- $spec
  label="case-$1-$2-$3"
  echo "$label" > current
  ./moss --profile "$1" "$2" "$3" > "$label.log" 2>&1
  status=$?; echo "$status" > "$label.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
sync
echo done > complete
