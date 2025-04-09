#!/bin/bash

# Compilation...

export TARGET="SDBOOT"
export GRPDIR="CONFIG"
export MEMMAP="noflash"

../../../_c1.sh "$1"
