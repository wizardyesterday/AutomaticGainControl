//**********************************************************************
// file name: AutomaticGainControl.h
//**********************************************************************

#ifndef _AUTOMATICGAINCONTROL_H_
#define _AUTOMATICGAINCONTROL_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

void agc_setOperatingPoint(int32_t operatingPointInDbFs);
int agc_setAgcFilterCoefficient(float coefficient);
int agc_setType(uint32_t type);
int agc_setDeadband(uint32_t deadbandInDb);
int agc_setBlankingLimit(uint32_t blankingLimit);
int agc_enable(void);
int agc_disable(void);
int agc_isEnabled(void);
void agc_acceptData(int32_t signalIndBFs);
void agc_runHarris(uint32_t signalMagnitude);
void agc_displayInternalInformation(void);

#endif // _AUTOMATICGAINCONTROL_H_
