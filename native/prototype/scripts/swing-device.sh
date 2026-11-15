#!/bin/sh
# Isolated functional and full-load checks; user projects remain unopened.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Swing-v1 || exit 2
. ./cpu-performance.sh || exit 3
./swing-arm > unit.log 2>&1
status=$?; echo "$status" > unit.exit
[ "$status" -eq 0 ] || exit "$status"
for mask in 18 17 24 3 10 5 9 6 12 20; do
  echo "swing / full load / FX pair $mask" > current
  ./moss --profile 17 "$mask" 20 > "pair-$mask.log" 2>&1
  status=$?; echo "$status" > "pair-$mask.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
for spec in '17 18 180' '18 18 60' '19 18 60' '20 18 60'; do
  set -- $spec
  label="case-$1-$2-$3"
  echo "$label" > current
  ./moss --profile "$1" "$2" "$3" > "$label.log" 2>&1
  status=$?; echo "$status" > "$label.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
sync
echo done > complete
