#!/bin/sh
#*****************************************************************************
# This build script creates an RtlSdr app.  It assumes that librtlsdr
# already exists in the ./lib directory.  If it does not exist, type
# ./buildRtlSdrLib.sh.
# Chris G. 05/21/2017
#*****************************************************************************
Executable="bin/rtl_app"

# Compile string.
Compile="g++ -o $Executable -Iinclude -I/usr/include/libusb-1.0"

# Link options
LinkOptions="-L lib -lrtlsdr -lusb-1.0"

# Build our application.
$Compile src/rtl_example.c $LinkOptions

# We're done.
exit 0

