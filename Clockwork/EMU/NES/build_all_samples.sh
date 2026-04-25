#!/bin/bash
for file in ./samples/*.NES; do
    [ -e "$file" ] || continue
    filename=$(basename "$file")
    name_only="${filename%.*}"
    ./_c_bulk.sh "$name_only"
done
