#!/bin/sh
# Install as Garlic's one-shot command.sh only when no pending command exists.
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH
export SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Granular-Lab || exit 2
. ./cpu-performance.sh || exit 3
FAILED=0
run_case() {
  label="live-$1-$2-$3"
  ./device-arm "$1" "$2" "$3" "$4" "$label.json" > "$label.log" 2>&1
  result=$?
  echo "$result" > "$label.exit"
  [ "$result" -eq 0 ] || FAILED=1
}
if [ "$#" -eq 4 ]; then
  run_case "$@"
elif [ "$#" -eq 0 ]; then
  run_case 8 10 3 600
  run_case 8 3 3 60
  run_case 8 10 1 60
  run_case 8 10 2 60
else
  exit 2
fi
sync
exit "$FAILED"
