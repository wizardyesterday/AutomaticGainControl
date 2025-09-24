#!/bin/sh
#*****************************************************************************
# This build script creates an RtlSdr app.  It assumes that all the libraries
# have been built already.  If they have not been built type ./buildLibs.sh.
# Chris G. 07/05/2017
#
# All reference to transmit functionality has been removed.
# Chris G. 10/24/2020
#*****************************************************************************
Executable="bin/radioDiags"

CcFiles="\
    src_diags/radioApp.cc \
    src_diags/DataConsumer.cc \
    src_diags/FrequencyScanner.cc \
    src_diags/FrequencySweeper.cc \
    src_diags/DbfsCalculator.cc \
    src_diags/IqDataProcessor.cc \
    src_diags/Radio.cc \
    src_diags/Squelch.cc \
    src_diags/SignalDetector.cc \
    src_diags/SignalTracker.cc \
    src_diags/console.cc \
    src_diags/diagUi.cc \
    src_diags/UdpClient.cc \
    src_diags/MessageQueue.cc"

Includes="\
    -I include \
    -I src/convenience \
    -I hdr_diags \
    -I Filters \
    -I Filters/Int16 \
    -I AmDemodulator \
    -I FmDemodulator \
    -I WbFmDemodulator \
    -I SsbDemodulator \
    -I AutomaticGainControl/include"
 
# Compile string.
Compile="g++ -O3 -o $Executable $Includes $CcFiles"

# Link options
LinkOptions="\
    -O3 \
    -L lib -lrtlsdr \
    -L lib -lAmDemodulator \
    -L lib -lFmDemodulator \
    -L lib -lWbFmDemodulator \
    -I SsbDemodulator \
    -L lib -lSsbDemodulator \
    -L lib -lAutomaticGainControl \
    -lusb-1.0 \
    -lm \
    -lpthread \
    -lrt"

# Build our application.
$Compile  $LinkOptions

# We're done.
exit 0

