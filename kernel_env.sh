#!/bin/bash

# LIULIU-ZYNQ1 / PetaLinux 2018.3 external kernel-module build environment.
# Usage:
#   source ~/petalinux/v2018.3/settings.sh
#   source ~/linux_demo/kernel_env.sh

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export PETALINUX_PROJECT="$HOME/petalinux/v2018.3/LIULIU-ZYNQ1"

# Find the actual kernel build directory produced by PetaLinux/Yocto.
# Keeping this discovery here avoids copying the long versioned path into
# every driver Makefile and tolerates small recipe/path changes after rebuilds.
_kdir_candidates=$(find "$PETALINUX_PROJECT/build/tmp/work" \
    -type f -name Module.symvers \
    -path '*/linux-xlnx/*/linux-plnx_zynq7-standard-build/Module.symvers' \
    2>/dev/null)

KDIR=$(printf '%s\n' "$_kdir_candidates" | head -n 1 | xargs -r dirname)
unset _kdir_candidates

if [ -z "$KDIR" ] || [ ! -f "$KDIR/Makefile" ]; then
    echo "ERROR: PetaLinux kernel build directory was not found." >&2
    echo "Build the kernel first, then source kernel_env.sh again." >&2
    unset KDIR
    return 1 2>/dev/null || exit 1
fi

export KDIR

echo "Kernel module build environment:"
echo "  ARCH=$ARCH"
echo "  CROSS_COMPILE=$CROSS_COMPILE"
echo "  PETALINUX_PROJECT=$PETALINUX_PROJECT"
echo "  KDIR=$KDIR"
echo

if command -v "${CROSS_COMPILE}gcc" >/dev/null 2>&1; then
    echo "Compiler: $(command -v "${CROSS_COMPILE}gcc")"
    "${CROSS_COMPILE}gcc" --version | head -n 1
else
    echo "WARNING: ${CROSS_COMPILE}gcc is not in PATH." >&2
    echo "Source the PetaLinux settings.sh first." >&2
fi
