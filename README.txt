0.0 Introduction

An Automatic Gain Control (AGC) is a feeback system that adjusts the
gain of a variable-gain amplifier (VGA) to maintain an operating point
such as a voltage magnitude level, current magnitude level, or in the case
of a digital radio, the magnitude of signal samples presented to the AGC.
Typically, an average magnitude of a block of data is used to perform a
smoothing action to the input provided to the AGC.

An attempt was made to make an accurate implementation of what was
described in the paper (by Fred Harris and Gregory Smith) in the doc/papers/
directory.  For details on design and implementation, refer to that paper.

1.0 System Requrements

The AGC software has been successfully run on a 300MHz Pentium II computer,
and on a BeagleBone Black with a 1GHz single-core ARM v7 processor.

The toolchain used for the Pentium II computer is gcc version 3.3 with
the associated linker.

The software has been built and tested in a Linux environment, and I don't
plan to support Microsoft Windows systems.  I welcome users to add the
appropriate macros to support Microsoft Windows.

This software *should* be able to build and execute on 64-bit processer
architectures.

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
This script builds the executable code *after* the libraries are built.

2.1.6 doc/
This directory contains all documentation that is related to AGC theory
and technology.

2.1.6.1 papers/
This directory contains technical papers related to AGC's.

2.1.6.2 agc_paper__10700.pdf

This paper, written by Fred Harris and Grgory Smith, describes the
design and implementation of a micropeocessor-controled AGC for a digital
receiver.  My AGC is an implementation of that described in the paper.

2.2 include/
This directory contains the header files listed below.

2.2.1 AutomaticGainControl.h

2.2.2 dbfsCalculator.h
This file is used internally by the AGC.

2.3 src/
This directory contains the header files listed below.

2.3.1 AutomaticGainControl.c

2.3.2 dbfsCalculator.c
This file is used internally by the AGC.

2.3.3 testAgc.cc
This program is compiled and used for unit testing. It is not used when
building an application.

2.4 lib/
This diectory contains the AGC library.

2.5 bin/
This directory contains the executable code.

3.0 API Description
The Application Progamming Interface (API) and a quick description of the
semantics and usage of each function usage is described below.
More detaileddescriptions are provided in the comment header blocks in the
file, AutomaticGainControl.c.

3.1 int agc_init(int32_t operatingPointInDbFs,
    uint32_t signalMagnitudeBitCount,
    void (*setGainCallbackPtr)(uint32_t gainIndB),
    uint32_t (*getGainCallbackPtr)(void))

This function initializes the AGC.
The operatingPointIndBFs parameter sets the initial operating point of the AGC.

The "signalMagnitudeBitCount" parameter dictates how many bits are required for
the signal magnitude.  This parameter is required so that the AGC can convert
the signal magnitude to units of decibels referenced to full scale, due to
the fact that the operating point is in decibels referanced to full scale.

The "setGainCallbackPtr" parameter is a pointer to an application-supplied
function that does the heavy lifting in order to set amplifier gain.

The "getGainCallbackPtr" parameter is a pointer to an application-supplied
function that does the heavy lifting in order to retrieve the  amplifier
gain that it may have set without the knowledge of the AGC's idea of the
gain. Given that the AGC was designed to operate in a multi-threaded
environment, data inconsistancy may occur.  Invoking thid callback function,
by the AGC, reduces the likelihood of data inconsistancy.
Let it be noted that the AGC is robust enough to compensate  for the
undesired effects of data inconsistancy.

3.2 void agc_setOperatingPoint(int32_t operatingPointInDbFs)

This function sets the operating point of the AGC in units of decibels
reference to full scale.
The "operatingPointInDbFs" has units of dBFs. A value of 0 impliies the
maximum full scale value.
In general, all values are <= 0, but positive values can be used for test
purposes.  The AGC will adjust the amplifier gain to achieve the operating
point (commonly called the set point).

3.3 int agc_setAgcFilterCoefficient(float coefficient)

This function sets the filter coefficient of the AGC.  The "coefficient"
parameter determines how quickly the AGC algorithm will converge to the
operatingpoint.  Valid values are 0 < coefficient < 1. Smaller values result
 in the AGC converging more slowly, and larger values result in more rapid
convergence to the operating point.

3.4 int agc_setDeadband(uint32_t deadbandInDb)

This function sets the deadband of the AGC.  The "deadbandInDb" parameter is
in unita of decibwels.  If the deadband is set to 0, any input signal change
will result in the AGC making an adjustment. If the deadband were 1, for
example, a signal change would have to be greater than +-1dB for the AGC to make a
gain adjustment.

3.5 int agc_setBlankingLimit(uint32_t blankingLimit)

This function sets the blanking timeout of the AGC.  The "blankingLimit"
parameter is in units of incoxations of the AGC algorithm.  The blanking
timeout provides a mechanism to inhibit the AGC from changing amplifier
gain until hardware settling time constraints are met.  In other words,
hardware transients must diminish before another gain adjustments are made,
otherwie, instability may occur.  The blanking limit determines the number
 of AGC invocations must occur before a gain adjustment can be made.  A
value of 0 will result in the AGC algorithm running whenever a signal change
occurs.  As an example a value of 1 will result in the AGC algorithm running
every other time.

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

This function is invoked whenever it is desired to run the AGC algorithm.
It is the single entry point that is used by he calling application code.
The "signalMagnitude" parameter represents the magnitude of the signal
that is presented to the AGC algorithm.

3.10 void agc_displayInternalInformation(char **displayBufferPtrPtr)

This function allows the calling function to display interesting operational
information about the AGC.  The callinf function passes the address of a
pointer to a buffer that is filled with a large zero-terminated C language
string.  The buffer can be displayed via printf("%s",theBuffer).


4.0 How to Build

4.1 Building the Example Code.
To build the teat software using testAgc.cc, perform the steps illustrated
below.

1. Navigate to the AutomaticGainControl directory
2. Type sh buildLibs.sh.
3. Type sh buildTestAgc.

You will now have an executable in the bin directory called testAgc.
The program does nothing other than initialize the AGC and display AGC
configuration and state information.  The file, testAgc.cc shows you how
to define your *static* callbacks which would normally reside in your
application code.
I used this program to unit-test the AGC code using a debugger.  You can add
bells and whistles to the test program so that you can get a feel of the
AGC code.
Test program output is illustrated below.

--------------------------------------------
AGC Internal Information
--------------------------------------------
AGC Emabled                : No
Blanking Counter           : 0 ticks
Blanking Limit             : 1 ticks
Lowpass Filter Coefficient : 0.800
Deadband                   : 1 dB
Operating Point            : -12 dBFs
Gain                       : 24 dB
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Signal Magnitude           : 0
RSSI (Before Amp)          : -24 dBFs
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/



4.2 Integrating the AGC Code into Your Application
To integrate the AGC code into your application code you have a few choices
which I'll discuss.

4.2.1 Choice 1

1. Copy the contents of the AutomaticGainControl directory into your
build directory.

2. Set up your build environment to reference the files, AutomaticGainContro.h
dbfsCalculator.h, AutomaticGainControl.cc, and dbFsCalculator.c

3. Provide a *static* callback function that sets the gain, in decibels, of
whatever amplifier you going to control.  I recommend controlling the
amplifier that drives the A/D converter in your receiver.  Additionally,
save the amplifier gain in a variable: for example, call it gainValue.

4. Provide a *static* callback function that returns the gain, in decibels,
of the amplifier that is bbeing controlled.  Let it return the  example
variable, gainValue, from the above example.

4.2.2 Choice 2

1. Copy files, AutomaticGainControl.h dbfsCalculator.h in an include directory
of your choice.

2. Copy files, AutomaticGainControl.c and dbfsCalculator.c  into a source
directory of your choice.

3. Set up your build environment to reference the files, AutomaticGainContro.h
dbfsCalculator.h, AutomaticGainControl.cc, and dbFsCalculator.c

4. Provide a *static* callback function that sets the gain, in decibels, of
whatever amplifier you going to control.  I recommend controlling the
amplifier that drives the A/D converter in your receiver.  Additionally,
save the amplifier gain in a variable: for example, call it gainValue.

5. Provide a *static* callback function that returns the gain, in decibels,
oamplifier that is being controlled.  Let it return the  example
variable, gainValue, from the above example.

5.0 Example Program
An example program is provided in src/testAgc.cc.  The program illustrates
how to initialize the AGC software with some contrived values, it explains
how to invoke the  AGC usng a contrived aignal magnitude, and an explanation
of when the AGC should actually be operated at run time.

6.0 Closing Remarks.

This document has provided a description of an implementation of an AGG
algorithm that was written in the C programming language.  The advanage
to using C is that the algorithm can be used for both C programs as well
as C++.

My motivation for creating an AGC was to give people the ability to run
SDR software on radios which conain A/D converters that produce 8-bit
output samples.  With a 48dB (theoretically, ignoring implementation loss),
you don't have much to work with in a radio environment with radically
different signal strengths.  With an amplifier, whose output drives an A/D
converter, on the rtl-sdr, when I listened to aircraft frequencies, I
would hear strong tones when a strong sixnal would be received.  The
solution was to reduce the LNA and mixer gains. I asked myself, why would I
want to reduce front-end sensitivity when signal overload was not occuring
at the variable gain amplifer input?  It was A/D converter overload!  With
an AGC, the user can establish a safe operating point that allows enough
headroom to avoid overload when a strong signal arrives.  When the signal
goes away, the gain is increased so that you can hear weak signals.

I have used an AGC on both the HackRF, and the rtl-sdr.  It has worked
well on both radios.

Even chips like the Analog Devices AD9361, provide hardware AGC functionality,
and that device produces 12-bit samples.

I have worked so long with radios, that I have become biased towards
radios having an AGC.  You get lots of bang for the buck.

It is my hope that some people, at the very least, look at my code.  They
might actually find it to be useful.

There are two points that I should iention.:
1. Do not enable the AGC until the receiver is started/enabled and the
receiver has stabilized.
2. The AGC can be disabled if it is desired to manually adjust the amplifier
gain.

I can be reached on the Libera IRC server in the ##rtlsdr channel, and
I will always answer any questions, and I will always help people out
in integrating this code into their applications.

And remember: this AGC is hardware-agnodtic!

Take care,
Chris
IRC nick: wizardyesterday
