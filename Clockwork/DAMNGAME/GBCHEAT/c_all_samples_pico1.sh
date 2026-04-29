#!/bin/bash

rem MAG ROM size: Picopad1 with RP2040
export PICO_DEVICE="clockwork10"
export GBMAXROM="1980000"

echo "GBMAXROM=$GBMAXROM"

for file in ./samples/*.GB; do
    [ -e "$file" ] || continue
    
    filename=$(basename "$file")
    name_only="${filename%.*}"
    
    ./_c_1_gb.sh "$name_only"
done

for file in ./samples/*.GBC; do
    [ -e "$file" ] || continue
    
    filename=$(basename "$file")
    name_only="${filename%.*}"
    
    ./_c_1_gbc.sh "$name_only"
done
