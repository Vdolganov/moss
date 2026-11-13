#!/bin/sh
PATH=/bin:/usr/bin:/sbin:/usr/sbin
export PATH SDL_NOMOUSE=1
cd /mnt/mmc/MOSS-Granular-Native || exit 2
. ./cpu-performance.sh || exit 3
./granular-integration-arm > final-integration.log 2>&1
MOSS_TEST_STATUS=$?
echo "$MOSS_TEST_STATUS" > final-integration.exit
[ "$MOSS_TEST_STATUS" -eq 0 ] || exit "$MOSS_TEST_STATUS"
./moss-final --profile 5 10 120 > final-worst.log 2>&1
echo "$?" > final-worst.exit
./moss-final --profile 7 10 60 > final-song.log 2>&1
echo "$?" > final-song.exit
sync
