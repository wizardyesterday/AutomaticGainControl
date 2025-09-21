//**********************************************************************
// file name: IqDataProcessor.h
//**********************************************************************

#ifndef _IQDATAPROCESSOR_H_
#define _IQDATAPROCESSOR_H_

#include <stdint.h>
#include "AmDemodulator.h"
#include "FmDemodulator.h"
#include "WbFmDemodulator.h"
#include "SsbDemodulator.h"
#include "Squelch.h"
#include "UdpClient.h"

class IqDataProcessor
{
  public:

  enum demodulatorType {None=0, Am=1, Fm=2, WbFm = 3, Lsb = 4, Usb = 5};

  IqDataProcessor(char *hostIpAddress,int hostPort);
  ~IqDataProcessor(void);

  void setDemodulatorMode(demodulatorType mode);
  void setAmDemodulator(AmDemodulator *demodulatorPtr);
  void setFmDemodulator(FmDemodulator *demodulatorPtr);
  void setWbFmDemodulator(WbFmDemodulator *demodulatorPtr);
  void setSsbDemodulator(SsbDemodulator *demodulatorPtr);
  void setSignalDetectThreshold(int32_t threshold);

  void downconvertByFsOver4(int8_t *bufferPtr,uint32_t byteCount);
  void upconvertByFsOver4(int8_t *bufferPtr,uint32_t byteCount);

  void acceptIqData(unsigned long timeStamp,
                    unsigned char *bufferPtr,
                    unsigned long byteCount);

  void enableSignalNotification(void);
  void disableSignalNotification(void);

  void registerSignalStateCallback(
      void (*signalCallbackPtr)(bool signalPresent,void *contextPtr),
      void *contextPtr);


  void enableSignalMagnitudeNotification(void);
  void disableSignalMagnitudeNotification(void);

  void registerSignalMagnitudeCallback(
      void (*callbackPtr)(uint32_t signalMagnitude,void *contextPtr),
      void *contextPtr);

  void enableIqDump(void);
  void disableIqDump(void);
  bool isIqDumpEnabled(void);

  void displayInternalInformation(void);

private:

  // Attributes.

  // The demodulator operating mode.
  demodulatorType demodulatorMode;

  // This is used for the squelch system.
  int32_t signalDetectThreshold;

  // Demodulator support.
  AmDemodulator *amDemodulatorPtr;
  FmDemodulator *fmDemodulatorPtr;
  WbFmDemodulator *wbFmDemodulatorPtr;
  SsbDemodulator *ssbDemodulatorPtr;

  // Squelch support.
  Squelch *squelchPtr;

  // IQ dump network support.
  UdpClient *networkInterfacePtr;
  bool iqDumpEnabled;

  // Signal notification support.
  bool signalNotificationEnabled;
  void *signalCallbackContextPtr;
  void (*signalCallbackPtr)(bool signalPresent,void *contextPtr);

  // Signal magnitude notification support.
  bool signalMagnitudeNotificationEnabled;
  void *signalMagnitudeCallbackContextPtr;
  void (*signalMagnitudeCallbackPtr)(uint32_t signalMagnitude,
                                     void *contextPtr);
};

#endif // _IQDATAPROCESSOR_H_
