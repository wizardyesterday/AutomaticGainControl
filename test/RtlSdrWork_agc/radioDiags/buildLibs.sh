#!/bin/sh
#*****************************************************************************
# This build script creates all of static libraries that can be used for
# creating RtlSdr apps.  To run this script, type ./buildLibs.sh.
# Chris G. 08/18/2017
#*****************************************************************************
sh buildAmDemodulatorLib.sh
sh buildFmDemodulatorLib.sh
sh buildWbFmDemodulatorLib.sh
sh buildSsbDemodulatorLib.sh
sh buildAgcLib.sh
sh buildRtlSdrLib.sh

exit 0
