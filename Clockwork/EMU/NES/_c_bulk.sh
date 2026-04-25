#!/bin/bash

echo ""

# goto stop
if [ -z "$1" ]; then
    exit 0
fi

ROM_NAME="$1"

rm -f src/program.cpp
rm -f src/setup.h

rm -f build/program.o
rm -f build/main.o
rm -f build/InfoNES.o
rm -f build/K6502_rw.o

# Odkomentuj následující řádky, pokud chceš kompilovat s méně používanými přepínači:
# rm -f build/InfoNES_Mapper.o
# rm -f build/NES_APU.o
# rm -f build/emu_nes_menu.o

if [ ! -f "samples/${ROM_NAME}.H" ]; then
    cp setup.h "samples/${ROM_NAME}.H" 2>/dev/null
fi

echo "${ROM_NAME}.nes"

./NESprep/NESprep "samples/${ROM_NAME}.NES" "src/program.cpp" "${ROM_NAME}"

cp "samples/${ROM_NAME}.H" "src/setup.h" 2>/dev/null

./c.sh ${PICO_DEVICE}

cp NES.uf2 "samples/${ROM_NAME}.UF2" 2>/dev/null

if [ ! -f "samples/${ROM_NAME}.BMP" ]; then cp NES.BMP "samples/${ROM_NAME}.BMP" 2>/dev/null; fi
if [ ! -f "samples/${ROM_NAME}.PNG" ]; then cp NES.PNG "samples/${ROM_NAME}.PNG" 2>/dev/null; fi
if [ ! -f "samples/${ROM_NAME}.TXT" ]; then cp NES.TXT "samples/${ROM_NAME}.TXT" 2>/dev/null; fi

if grep -q "CRC=" "samples/${ROM_NAME}.TXT" 2>/dev/null; then
    # if not errorlevel 1 goto stop
    exit 0
fi

./NESprep/MapperNo/MapperNo "samples/${ROM_NAME}.NES" >> "samples/${ROM_NAME}.TXT"

if [ $? -eq 0 ]; then
    exit 0
fi

echo "${ROM_NAME}.NES error!"
read -p "Stiskněte Enter pro pokračování..."
