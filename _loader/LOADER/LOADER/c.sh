#!/bin/bash

# Compilation...
if [ "$2" == "" ]; then
    echo "Missing architecture format ... 0=RP2040, 1=RP2350-ARM, 2=RP2350-RISCV"
    exit 1
fi

export TARGET="LOADER"
export GRPDIR="."
export MEMMAP=""

../../../_c1.sh "$1"

# Export to ASM
exe/LoaderBin LOADER.bin ../../loader_"$1".S "$2"
