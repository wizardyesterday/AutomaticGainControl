#!/bin/sh
#*****************************************************************************
# This build script creates an AGC app.  It assumes that all the libraries
# have been built already.  If they have not been built type ./buildLibs.sh.
# Chris G. 09/16/2025
#*****************************************************************************

Executable="bin/testAgc"

CcFiles="\
    src/testAgc.cc"

Includes="\
    -I include"
 
# Compile string.
Compile="g++ -g -O0 -o $Executable $Includes $CcFiles"

# Link options
LinkOptions="\
    -O0 \
    -L lib -lAutomaticGainControl \
    -lm"

# Build our application.
$Compile  $LinkOptions

# We're done.
exit 0
