1.0 Inttroduction

2.0 Directory Structure and File Descriptions
The directory structure and the descriptions are provided below.

2.1 doc/papers
This directory contains useful papers related to AGC's.

2.1.1 agc_paper__10700.pdf
This paper, written by Fred Harris and Grgory Smith, describes the
design and implementation of a micropeocessor-controled AGC for a digital
receiver.  My AGC is an implementation of that described in the paper.

3.0 API Description
The Application Progamming Interface (API) and a quick description of the
semantics and usage of each functio usage is described below.

3.1 int agc_init(int32_t operatingPointInDbFs,
    uint32_t signalMagnitudeBitCount,
    void (*setGainCallbackPtr)(uint32_t gainIndB),
    uint32_t (*getGainCallbackPtr)(void))

3.2 void agc_setOperatingPoint(int32_t operatingPointInDbFs)

3.3 int agc_setAgcFilterCoefficient(float coefficient)

3.4 int agc_setDeadband(uint32_t deadbandInDb)

3.5 int agc_setBlankingLimit(uint32_t blankingLimit)

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



4.0 How to Build

5.0 Closing Remarks.


