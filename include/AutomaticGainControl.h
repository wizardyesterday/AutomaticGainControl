//**********************************************************************
// file name: AutomaticGainControl.h
//**********************************************************************

#ifndef _AUTOMATICGAINCONTROL_H_
#define _AUTOMATICGAINCONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

 void agc_init(int32_t operatingPointInDbFs,
 void (*gainSetCallbackPtr)(uint32_t gainIndB),
 uint32_t (*gainGetCallbackPtr)(void));

void agc_setOperatingPoint(int32_t operatingPointInDbFs);
int agc_setAgcFilterCoefficient(float coefficient);
int agc_setDeadband(uint32_t deadbandInDb);
int agc_setBlankingLimit(uint32_t blankingLimit);
int agc_enable(void);
int agc_disable(void);
int agc_isEnabled(void);
void agc_acceptData(int32_t signalIndBFs);
void agc_runHarris(int32_t signalIndBFs);
void agc_displayInternalInformation(void);

#ifdef __cplusplus
}
#endif

#endif // _AUTOMATICGAINCONTROL_H_
