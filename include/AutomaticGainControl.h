//**********************************************************************
// file name: AutomaticGainControl.h
//**********************************************************************

#ifndef _AUTOMATICGAINCONTROL_H_
#define _AUTOMATICGAINCONTROL_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// New AGC algorithms will be added as time progresses.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
#define AGC_TYPE_LOWPASS (0)
#define AGC_TYPE_HARRIS (1)
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

void agc_setOperatingPoint(int32_t operatingPointInDbFs);
int agc_setAgcFilterCoefficient(float coefficient);
int agc_setType(uint32_t type);
int agc_setDeadband(uint32_t deadbandInDb);
int agc_setBlankingLimit(uint32_t blankingLimit);
int agc_enable(void);
int agc_disable(void);
int isEnabled(void);
void agc_run(uint32_t signalMagnitude);
void runLowpass(uint32_t signalMagnitude);
void agc_runHarris(uint32_t signalMagnitude);
void agc_displayInternalInformation(void);

#endif // _AUTOMATICGAINCONTROL_H_
