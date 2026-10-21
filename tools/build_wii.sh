#!/bin/bash
set -e

# WiiWaker Build Script
export DEVKITPRO="/opt/devkitpro"
export DEVKITPPC="$DEVKITPRO/devkitPPC"
export PATH="$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"

if [ ! -d "$DEVKITPPC" ]; then
    echo "=========================================================="
    echo " devkitPPC not found in $DEVKITPPC"
    echo " Please install wii-dev first:"
    echo "   sudo dkp-pacman -S wii-dev"
    echo "=========================================================="
    exit 1
fi

echo "=========================================================="
echo " Building WiiWaker (.dol) with devkitPPC..."
echo "=========================================================="
cd "$(dirname "$0")/.."
make clean
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
make -j"$JOBS"

echo ""
echo "=========================================================="
echo " Build successful! Binary generated at: WiiWaker.dol"
echo "=========================================================="
