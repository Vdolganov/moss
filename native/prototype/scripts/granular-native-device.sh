#!/bin/sh
# Run through Garlic's one-shot command when MOSS is closed.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Granular-Native || exit 2
. ./cpu-performance.sh || exit 3
for spec in '5 3 60' '5 5 30' '5 9 30' '5 6 30' '5 12 30' '5 10 600' '8 10 60' '9 10 60' '6 10 120' '7 10 180'; do
  set -- $spec
  label="native-$1-$2-$3"
  ./moss --profile "$1" "$2" "$3" > "$label.log" 2>&1
  echo "$?" > "$label.exit"
done
sync
