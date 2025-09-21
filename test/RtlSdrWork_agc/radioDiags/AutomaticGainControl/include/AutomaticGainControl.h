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

int agc_init(int32_t operatingPointInDbFs,
    uint32_t signalMagnitudeBitCount,
    void (*setGainCallbackPtr)(uint32_t gainIndB),
    uint32_t (*getGainCallbackPtr)(void));

void agc_setOperatingPoint(int32_t operatingPointInDbFs);
int agc_setAgcFilterCoefficient(float coefficient);
int agc_setDeadband(uint32_t deadbandInDb);
int agc_setBlankingLimit(uint32_t blankingLimit);
int agc_enable(void);
int agc_disable(void);
int agc_isEnabled(void);
void agc_acceptData(uint32_t signalMagnitude);
void agc_displayInternalInformation(char **displayBufferPtrPtr);

#ifdef __cplusplus
}
#endif

#endif // _AUTOMATICGAINCONTROL_H_
