
# ASM source files
ASRC +=

# C source files
CSRC += lib/lib_video_clockwork.c

# C++ source files
SRC += src/main.cpp

# Path to root directory from the project directory (without trailing '/' delimiter)
PICO_ROOT_PATH = ../../..

# Makefile includes
include ${PICO_ROOT_PATH}/Makefile.inc
CSRCS := $(filter-out %lib_video.c, $(CSRCS))
