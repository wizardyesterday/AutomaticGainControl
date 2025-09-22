1.0 Inttroduction

2.0 Directory Structure and File Descriptions
The directory structure and the descriptions are provided below.

2.1 AutomatricGainControl/
This top level directory contains the build scripts and all of the
subdirectories.

2.1.1 LICENSE

2.1.2 README.txt
This file.

2.1.3 buildLibs.sh
This script builds all library code.

2.1.4 buildAgcLib.sh
This script builds The AGC library code.

2.1.5 buildTestAgc.sh
This scruot builds the executable code *after* the libraries are built.

2.1.6 doc/papers
This directory contains useful papers related to AGC's.

2.1.1.8 agc_paper__10700.pdf
This paper, written by Fred Harris and Grgory Smith, describes the
design and implementation of a micropeocessor-controled AGC for a digital
receiver.  My AGC is an implementation of that described in the paper.

2.2 test/
This directory contains the header files listed below.

2.2.1 RtlSdrWork_agc/
This directory contains all filess necessary to build a version of my
radio  diags used to validate operation of this AGC.


2.3 include/
This directory contains the header files listed below.

2.3.1 AutomaticGainControl.h

.3.2 dbfsCalculator.h

2.4 src/
This directory contains the header files listed below.

2.4.1AutomaticGainControl.c

2.4.2dbfsCalculator.c

2.4.3testAgc.cc

2.5 lib/
This diectory contains the AGC library.
c
2.6 bin/
This directory contains the executable code.

3.0 API Description
The Application Progamming Interface (API) and a quick description of the
semantics and usage of each functio usage is described below.

3.1 int agc_init(int32_t operatingPointInDbFs,
    uint32_t signalMagnitudeBitCount,
    void (*setGainCallbackPtr)(uint32_t gainIndB),
    uint32_t (*getGainCallbackPtr)(void))

This function initializes the AGC.
The operatingPointIndBFs parameter sets the initial operating point of the AGC.

The signalMagnitudeBitCount dictates how many bits are required for the
signal magnitude.  This parameter is required so that the AGC can convert
the signal magnitude to units of decibels referenced to full scale, due to
the fact that the operating point is in decibels referanced to full scale.

The setGainCallbackPtr parameter is a pointer to an application-supplied
function that does the heavy lifting in order to set amplifier gain.

The getGainCallbackPtr parameter is a pointer to an application-supplied
function that does the heavy lifting in order to retrieve the  amplifier
gain that it may have set without the knowledge of the AGC's idea of the
gain. Given that the AGC was designed to operate in a multi-threaded
environment, data inconsistancy may occur.  Invoking thid callback function,
by the AGC, reduces the likelihood of data inconsistancy.
Let it be noted that the AGC is robust enough to compensate  for the
undesired effects of data inconsistancy.

3.2 void agc_setOperatingPoint(int32_t operatingPointInDbFs)

This function sets the operating point of the AGC in units of decibels
reference to full scale. A value of 0 impliies the maximum full scale value.
In general, all values are <= 0, but positive values can be used for test
purposes.  The AGC will adjust the amplifier gain to achieve the operating
point (commonly called the set point).

3.3 int agc_setAgcFilterCoefficient(float coefficient)

This function sets the filter coefficient of the AGC.  The filter coefficient
determines how quickly the AGC algorithm will converge to the operating
point.  Valid values are 0 < coefficient < 1. Smaller values result in the
AGC converging more slowly, and larger values result in more rapid
convergence to the operating point.

3.4 int agc_setDeadband(uint32_t deadbandInDb)

This function sets the deadband of the AGC.  another name for deadband is
hysteresis. If the deadband is set to 0, any input signal change will
result in the AGC making an adjustment. If the deadband were 1, for example,
A signal change would have to be greater than +-1dB for the AGC to make a
gain adjustment.

3.5 int agc_setBlankingLimit(uint32_t blankingLimit)

This function sets the blanking timeout of the AGC. The blanking timeout
provides a mechanism to inhibit the AGC from changing amplifier gain until
hardware settling time constraints are met.  In other words, hardware
transients must diminish before another gain adjustments diminish, otherwie,
instability may occur.  The blanking limit determines the number of AGC
invocations must occur before a gain adjustment can be made.  A value of
1 will result in the AGC algorithm running every other time.

3.6 int agc_enable(void)

This function enables the AGC.  The receiver should be started, and time
should be allowed for it to stabilize before enabling the AGC.


3.7 int agc_disable(void)
This function disables the AGC.  This allows the user to manually control
the gain of the receiver.

3.8 int agc_isEnabled(void)

This function allows the AGC enable state to be queried.  A value of 1
indicates that the AGC is enabled, and a value of 0 indicates that the AGC
is disabled.

3.9 void agc_acceptData(uint32_t signalMagnitude)

3.10 void agc_displayInternalInformation(char **displayBufferPtrPtr)

This function allows the calling function to display interesting operational
information about the AGC.  The callinf function passes the address of a
pointer to a buffer that is filled with a large zero-terminated C language
string.  The buffer can be displayed via printf("%s",theBuffer).


4.0 How to Build

5.0 Closing Remarks.


