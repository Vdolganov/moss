#!/bin/sh
MOSS_BASE=$(CDPATH= cd "$(dirname "$0")" && pwd) || exit 1
cd "$MOSS_BASE/MOSS" || exit 1
mkdir -p projects reports || exit 1
MOSS_STAMP=$(date +%Y%m%d-%H%M%S)
exec >>"reports/startup-$MOSS_STAMP-$$.log" 2>&1
echo "MOSS 0.9.0 / original RG35XX / GarlicOS"
date
uname -a
chmod +x ./moss 2>/dev/null
. ./cpu-performance.sh || exit 1
./moss
MOSS_STATUS=$?
echo "Exit: $MOSS_STATUS"
sync
exit "$MOSS_STATUS"
