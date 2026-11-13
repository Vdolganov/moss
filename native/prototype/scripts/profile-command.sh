#!/bin/sh
# One-shot Garlic launcher: parent CFW/start removes this command and restores
# the menu after completion. Install only when command.sh does not exist.
cd /mnt/mmc/Roms/APPS/MOSS || exit 2
export SDL_NOMOUSE=1
. ./perf/cpu-performance.sh || exit 1
for spec in '0 10 120' '3 10 120' '-1 10 120' '2 10 600'; do
  set -- $spec
  ./perf/moss-profile --profile "$1" "$2" "$3" > "perf/live-async-$1.log" 2>&1
  echo "$?" > "perf/live-async-$1.exit"
done
sync
