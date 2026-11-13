#!/bin/sh
# Original RG35XX / GarlicOS 1.x. Keep this file beside the MOSS-LAB folder.
MOSS_BASE=$(CDPATH= cd "$(dirname "$0")" && pwd) || exit 1
MOSS_APP="$MOSS_BASE/MOSS-LAB"
cd "$MOSS_APP" || exit 1
mkdir -p reports || exit 1
MOSS_STAMP=$(date +%Y%m%d-%H%M%S)
MOSS_LOG="$MOSS_APP/reports/startup-$MOSS_STAMP-$$.log"
exec >>"$MOSS_LOG" 2>&1
echo "MOSS LAB 0.1.3 / target GarlicOS 1.3.9 / original RG35XX"
date
uname -a
echo "--- CPU ---"
cat /proc/cpuinfo
echo "--- MEMORY ---"
cat /proc/meminfo
echo "--- INPUT ---"
cat /proc/bus/input/devices
echo "--- AUDIO ---"
cat /proc/asound/cards 2>/dev/null
echo "--- SYSTEM LIBRARIES ---"
ls -l /lib/ld* /lib/libc* /lib/libuClibc* /usr/lib/libSDL* /usr/lib/libasound* /mnt/SDCARD/CFW/lib/libSDL* 2>/dev/null
echo "--- CPU FREQUENCY (READ ONLY) ---"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null
echo "--- DISPLAY ---"
cat /sys/class/graphics/fb0/virtual_size /sys/class/graphics/fb0/bits_per_pixel 2>/dev/null
echo "--- ENVIRONMENT ---"
echo "SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-auto} SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-auto}"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-default}"
echo "--- LAUNCH ---"
# FAT partitions commonly synthesize permissions; chmod is harmless if supported.
chmod +x ./moss-lab 2>/dev/null
./moss-lab "$MOSS_APP/reports"
MOSS_STATUS=$?
echo "--- EXIT $MOSS_STATUS ---"
date
sync
exit "$MOSS_STATUS"
