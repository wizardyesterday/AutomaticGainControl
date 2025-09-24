#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# RtlSdr apps.  To run this script, type ./buildRtlSdrLib.sh"
# Chris G. 05/17/2017
#*****************************************************************************
Compile="gcc -c -O3 -D DETACH_KERNEL_DRIVER=1 -Iinclude -I/usr/include/libusb-1.0"

# First compile the files of interest.
$Compile src/convenience/convenience.c
$Compile src/librtlsdr.c
$Compile src/tuner_e4k.c
$Compile src/tuner_fc0012.c
$Compile src/tuner_fc0013.c
$Compile src/tuner_fc2580.c
$Compile src/tuner_r82xx.c

# Create the archive.
ar rcs lib/librtlsdr.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

