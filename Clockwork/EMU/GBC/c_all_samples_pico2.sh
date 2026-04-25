#!/bin/bash

# MAG ROM size: Picopad2 s RP2350
export GBMAXROM="4000000"

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
