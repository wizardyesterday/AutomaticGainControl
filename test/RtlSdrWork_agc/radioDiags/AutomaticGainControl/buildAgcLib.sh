#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# an AGC library.  To run this script, type ./buildAgcLib.sh"
# Chris G. 09/16/2025
#*****************************************************************************
Compile="gcc -c -g  -O0 -Iinclude"

# First compile the files of interest.
$Compile src/AutomaticGainControl.c
$Compile src/dbfsCalculator.c

# Create the archive.
ar rcs lib/libAutomaticGainControl.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

