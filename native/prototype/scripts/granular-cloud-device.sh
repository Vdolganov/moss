#!/bin/sh
# Isolated cloud regression; run only while the user is in Garlic's menu.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Granular-Cloud-v2 || exit 2
. ./cpu-performance.sh || exit 3
./test-arm > unit.log 2>&1
status=$?; echo "$status" > unit.exit
[ "$status" -eq 0 ] || exit "$status"
./integration-arm > integration.log 2>&1
status=$?; echo "$status" > integration.exit
[ "$status" -eq 0 ] || exit "$status"
for spec in '5 10 600' '8 10 60' '9 10 60' '6 10 60' '7 10 60'; do
  set -- $spec
  label="cloud-$1-$2-$3"
  ./moss --profile "$1" "$2" "$3" > "$label.log" 2>&1
  status=$?
  echo "$status" > "$label.exit"
  [ "$status" -eq 0 ] || exit "$status"
done
sync
echo done > complete
