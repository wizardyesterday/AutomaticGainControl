//**********************************************************************
// file name: Radio.h
//**********************************************************************

#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "IqDataProcessor.h"
#include "DataConsumer.h"
#include "AmDemodulator.h"
#include "FmDemodulator.h"
#include "WbFmDemodulator.h"
#include "SsbDemodulator.h"

#define RADIO_RECEIVE_AUTO_GAIN (99999)

class Radio
{
  public:

  Radio(int deviceNumber,uint32_t rxSampleRate,
        char *hostIpAddress,int hostPort,
        void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength));

  ~Radio(void);

  // Setters.
  bool setReceiveFrequency(uint64_t frequency);
  bool setReceiveBandwidth(uint32_t bandwidth);
  bool setReceiveGainInDb(uint32_t gain);
  bool setReceiveIfGainInDb(uint8_t stage,uint32_t gain);
  bool setReceiveSampleRate(uint32_t sampleRate);
  bool setReceiveWarpInPartsPerMillion(int warp);
  bool setSignalDetectThreshold(int32_t threshold);

  // Getters.
  uint64_t getReceiveFrequency(void);
  uint32_t getReceiveBandwidth(void);
  uint32_t getReceiveGainInDb(void);
  uint32_t getReceiveIfGainInDb(void);
  uint32_t getReceiveSampleRate(void);
  int getReceiveWarpInPartsPerMillion(void);

  bool isReceiving(void);
  bool startReceiver(void);
  void stopReceiver(void);

  // Demodulator support.
  void setDemodulatorMode(uint8_t mode);
  void setAmDemodulatorGain(float gain);
  void setFmDemodulatorGain(float gain);
  void setWbFmDemodulatorGain(float gain);
  void setSsbDemodulatorGain(float gain);

  IqDataProcessor *getIqProcessor(void);
  void enableIqDump(void);
  void disableIqDump(void);
  bool isIqDumpEnabled(void);

  // AGC support.
  bool setAgcType(uint32_t type);
  bool setAgcDeadband(uint32_t deadbandInDb);
  bool setAgcBlankingLimit(uint32_t blankingLimit);
  void setAgcOperatingPoint(int32_t operatingPointInDbFs);
  bool setAgcFilterCoefficient(float coefficient);
  bool enableAgc(void);
  bool disableAgc(void);
  bool isAgcEnabled(void);
  void displayAgcInternalInformation(void);

  void displayInternalInformation(void);

  private:

  // Utility functions.
  bool setupReceiver(void);
  void tearDownReceiver(void);

  static void eventConsumerProcedure(void *arg);

  // Attributes.

  //------------------------------------------------------------
  // Receiver configuration.
  //------------------------------------------------------------
  uint64_t receiveFrequency;
  uint32_t receiveSampleRate;
  uint32_t receiveBandwidth;
  uint32_t receiveGainInDb;
  uint32_t receiveIfGainInDb;
  int receiveWarpInPartsPerMillion;

  // RtlSdr device support.
  int deviceNumber;
  void *devicePtr;

  // Buffer support.
  uint8_t *receiveBufferPtr;

  // Data handler support.
  IqDataProcessor *receiveDataProcessorPtr;
  DataConsumer *dataConsumerPtr;

  // Demodulator support.
  AmDemodulator *amDemodulatorPtr;
  FmDemodulator *fmDemodulatorPtr;
  WbFmDemodulator *wbFmDemodulatorPtr;
  SsbDemodulator *ssbDemodulatorPtr;

  // Control information.
  bool receiveEnabled;
  bool timeToExit;

  // Event consumer thread support.
  bool requestReceiverToStop;
  pthread_mutex_t radioLock;
  pthread_t eventConsumerThread;

  // Multi-thread support.
  pthread_mutex_t ioSubsystemLock;

  // Statistics.
  uint32_t receiveTimeStamp;
  uint32_t receiveBlockCount;
};

#endif // _RADIO_H_
