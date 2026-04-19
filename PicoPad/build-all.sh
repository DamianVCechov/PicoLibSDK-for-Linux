#!/bin/bash

ARG="$1"

find . -maxdepth 4 -name "src" -prune -o -type d -print | while read -r dir; do

    if [ "$dir" == "." ]; then continue; fi

    (
        cd "$dir" || exit

        if [ -x "./c.sh" ]; then
            echo "Spouštím v: $dir"
            ./c.sh "$ARG"
            ./d.sh
        fi
    )
done
