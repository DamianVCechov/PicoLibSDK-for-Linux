#!/bin/bash

for file in *; do
    [ -f "$file" ] || continue
    
    [ "$file" = "$(basename "$0")" ] && continue

    first_char="${file:0:1}"
    
    first_char="${first_char^^}"

    if [[ "$first_char" =~ [0-9] ]]; then
        target_dir="0-9"
    elif [[ "$first_char" =~ [A-Z] ]]; then
        target_dir="$first_char"
    else
        target_dir="Ostatni"
    fi

    mkdir -p "$target_dir"

    mv "$file" "$target_dir/"
done

echo "Třídění bylo dokončeno."
