#!/bin/bash

# Export result file to the hardware...
# Edit name of the destination disk in the "copy" command,
#   or change disk name of the Pico image in task manager.

if [[ -e "${TARGET}".uf2 ]]; then
  if [[ -e /media/"${USER}"/RPI-RP2 ]]; then
     cp "${TARGET}".uf2 /media/"${USER}"/RPI-RP2        # Change this name of the mount point folder of your RPI-RP2 or RP2350 here.
  else
     if [[ -e /media/"${USER}"/RP2350 ]]; then
       cp "${TARGET}".uf2 /media/"${USER}"/RP2350
     else
       echo "The device is not connected or mounted. It is expected in the /media/$USER/RPI-RP2 or /media/$USER/RP2350 directory";
       echo "For a different path, edit these paths in the _e1.sh file located in the main directory..";
     fi
  fi
fi
echo $DEVICE


