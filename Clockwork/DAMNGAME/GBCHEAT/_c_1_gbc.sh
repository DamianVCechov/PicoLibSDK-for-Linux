#!/bin/bash

if [ -z "$1" ]; then
    exit 0
fi

ROM_NAME="$1"

rm -f src/program.cpp
rm -f build/program.o

echo "${ROM_NAME}.GBC"

./GBprep/GBprep "samples/${ROM_NAME}.GBC" "src/program.cpp" "${ROM_NAME}" "GBC" "$GBMAXROM"

if [ $? -ne 0 ]; then
    # Zde jsme v bloku :err
    read -p "Stiskněte Enter pro pokračování..."
    exit 1
fi

./c.sh ${PICO_DEVICE}

cp GBCHEAT.uf2 "samples/${ROM_NAME}.UF2"

if [ ! -f "samples/${ROM_NAME}.BMP" ]; then
    cp GBCHEAT.BMP "samples/${ROM_NAME}.BMP"
fi

if [ ! -f "samples/${ROM_NAME}.PNG" ]; then
    cp GBCHEAT.PNG "samples/${ROM_NAME}.PNG"
fi

exit 0
