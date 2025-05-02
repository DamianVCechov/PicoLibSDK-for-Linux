#!/bin/bash

# compile all tools
cd Boot2CRC
gcc -m32 boot2crc.cpp -o boot2crc
cd ../HidComp
gcc -m32 HidComp.cpp -o HidComp
cd ../PicoPadImg
gcc -m32 PicoPadImg.cpp -o PicoPadImg
cd ../PicoPadImg2
gcc -m32 PicoPadImg.cpp -o PicoPadImg2
cd ../PicoPadLoaderBin
gcc -m32 LoaderBin.cpp -o LoaderBin
cd ../PicoPadLoaderCrc
gcc -m32 LoaderCrc.cpp -o LoaderCrc
cd ../PicoPadVideo
gcc -m32 PicoPadVideo.cpp -o PicoPadVideo
cd ../RaspPicoClock
gcc -m32 PicoClock.cpp -o PicoClock
cd ../RaspPicoImg
gcc -m32 RaspPicoImg.cpp -o RaspPicoImg
cd ../RaspPicoPal332
gcc -m32 pal332.cpp -o pall332
cd ../RaspPicoRle
gcc -m32 RaspPicoRle.cpp -o RaspPicoRle
cd ../RaspPicoSnd
gcc -m32 RaspPicoSnd.cpp -o RaspPicoSnd
cd ../DviTmds
gcc -m32 DviTmds.cpp -o DviTmds
cd ../DviTmds/DviTmds_AllTab
gcc -m32 DviTmds.cpp -o DviTmds
cd ../../BinC
gcc -m32 BinC.cpp -o BinC
cd ../BinS
gcc -m32 BinS.cpp -o BinS
cd ../BinAsm
gcc -m32 BinAsm.cpp -o BinAsm

cd ../elf2uf2
g++ -m32 main.cpp -o elf2uf2
