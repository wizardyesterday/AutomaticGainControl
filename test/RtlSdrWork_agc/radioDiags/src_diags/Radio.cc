//**********************************************************************
// file name: Radi
//**********************************************************************

#include <stdio.h>
#include <unistd.h>

#include "Radio.h"
#include "AutomaticGainControl.h"

extern "C"
{
  #include "rtl-sdr.h"
  #include "convenience.h"
}

#define RECEIVE_BUFFER_SIZE (32768)

// The current variable gain setting.
uint32_t radio_adjustableReceiveGainInDb;

extern void nprintf(FILE *s,const char *formatPtr, ...);

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// C language AGC support.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
static Radio *myRadioPtr;
static char *agcDisplayBufferPtr;

static void setGainCallback(uint32_t gainIndB)
{
  int status;

  status = myRadioPtr->setReceiveIfGainInDb(0,gainIndB);

  return;

} // setGainCallback

static uint32_t getGainCallback(void)
{

  return (radio_adjustableReceiveGainInDb);

} // getGainCallback

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// End of C language AGC support.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/


/*****************************************************************************

  Name: nullPcmDataHandler

  Purpose: The purpose of this function is to serve as the callback
  function to accept PCM data from a demodulator in the event that a
  callback is not passed to the constructor to an instance of a Radio.
  This prevents the demodulators from invocation a NULL address in this
  case.  This handler will merely discard any PCM data that is passed
  to it.

  Calling Sequence: nullPcmDataHandler(bufferPtr,bufferLength).

  Inputs:

    bufferPtr - A pointer to a buffer of PCM samples.

    bufferLength - The number of PCM samples in the buffer.

  Outputs:

    None.

*****************************************************************************/
static void nullPcmDataHandler(int16_t *bufferPtr,uint32_t bufferLength)
{

  // Discard the data.

  return;

} // nullPcmDataHandler

/**************************************************************************

  Name: Radio

  Purpose: The purpose of this function is to serve as the constructor
  of a Radio object.

  Calling Sequence: Radio(deviceNumber,rxSampleRate,
                          hostIpAddress,hostPort,pcmCallbackPtr)

  Inputs:

    deviceNumber - The device number of interest.  Just use 0.

    rxSampleRate - The sample rate of the baseband portion of the
    receiver in samples per second.

    hostIpAddress - A dotted decimal representation of the IP address
    of the host to support streaming of IQ data.

    hostPort - The UDP port for which the above mentioned host is
    listening.

    pcmCallbackPtr - A pointer to a callback function that is to
    process demodulated data.

  Outputs:

    None.

**************************************************************************/
Radio::Radio(int deviceNumber,uint32_t rxSampleRate,
             char *hostIpAddress,int hostPort,
             void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength))
{ 
  int i;
  bool success;
  pthread_mutexattr_t mutexAttribute;

  // Used by callbacks that support IF AGC.
  myRadioPtr = this;

  // Use by the AGC display function.
  agcDisplayBufferPtr = new char[65536];

  if (pcmCallbackPtr == NULL)
  {
    // Set the pointer to something sane.
    pcmCallbackPtr = nullPcmDataHandler;
  } // if

  // Save for later use.
  this->receiveSampleRate = rxSampleRate;

  // Save for later use.
  this->deviceNumber = deviceNumber;

  // Indicate that there is no association with a device.
  devicePtr = 0;

  // Ensure that we don't have any dangling pointers.
  receiveDataProcessorPtr = 0;
  dataConsumerPtr = 0;

  // Start with a clean slate.
  receiveTimeStamp = 0;
  receiveBlockCount = 0;

  // Indicate that the radio is not requested to stop.
  requestReceiverToStop = false;

  // Initialize radio lock.
  pthread_mutex_init(&radioLock,0);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Initialize the I/O subsystem lock.  This occurs
  // since some functions that lock the mutex invoke
  // other functions that lock the same mutex.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Set up the attribute.
  pthread_mutexattr_init(&mutexAttribute);
  pthread_mutexattr_settype(&mutexAttribute,PTHREAD_MUTEX_RECURSIVE);

  // Initialize the mutex using the attribute.
  pthread_mutex_init(&ioSubsystemLock,&mutexAttribute);

  // This is no longer needed.
  pthread_mutexattr_destroy(&mutexAttribute);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Set up the receiver with a default configuration.
  success = setupReceiver();

  if (success)
  {
    // Allocate the receive buffer that is to be used for IQ samples.
    receiveBufferPtr = (uint8_t *)calloc(RECEIVE_BUFFER_SIZE,sizeof(uint8_t));

    // Instantiate the IQ data processor object.  The data consumer object
    // will need this.
    receiveDataProcessorPtr = new IqDataProcessor(hostIpAddress,hostPort);

    // Instantiate the data consumer object.  eventConsumerProcedure method
    // will need this.
    dataConsumerPtr = new DataConsumer(receiveDataProcessorPtr);

    // Instantiate the AM demodulator.
    amDemodulatorPtr = new AmDemodulator(pcmCallbackPtr);

    // Associate the AM demodulator with the Iq data processor.
    receiveDataProcessorPtr->setAmDemodulator(amDemodulatorPtr);

    // Instantiate the FM demodulator.
    fmDemodulatorPtr = new FmDemodulator(pcmCallbackPtr);

    // Associate the FM demodulator with the Iq data processor.
    receiveDataProcessorPtr->setFmDemodulator(fmDemodulatorPtr);

    // Instantiate the wideband FM demodulator.
    wbFmDemodulatorPtr = new WbFmDemodulator(pcmCallbackPtr);

    // Associate the FM demodulator with the Iq data processor.
    receiveDataProcessorPtr->setWbFmDemodulator(wbFmDemodulatorPtr);

    // Instantiate the SSB demodulator.
    ssbDemodulatorPtr = new SsbDemodulator(pcmCallbackPtr);

    // Associate the SSB demodulator with the Iq data processor.
    receiveDataProcessorPtr->setSsbDemodulator(ssbDemodulatorPtr);

    // Set the demodulation mode.
    receiveDataProcessorPtr->setDemodulatorMode(IqDataProcessor::Fm);

    // Initialize the AGC with an operating point of -12dBFs.
    // and a signal magnitude size of 7 bits.
    success = agc_init(-12,7,setGainCallback,getGainCallback);

    // Create the event consumer thread.
    pthread_create(&eventConsumerThread,0,
                   (void *(*)(void *))eventConsumerProcedure,this);
  } // if
  else
  {
    fprintf(stderr,"Could not initialize receiver.\n");
  } // else
 
  return;
 
} // Radio

/**************************************************************************

  Name: ~Radio

  Purpose: The purpose of this function is to serve as the destructor
  of a Radio object.

  Calling Sequence: Radio()

  Inputs:

    None.

  Outputs:

    None.


**************************************************************************/
Radio::~Radio(void)
{

  // Notify the event consumer thread that it is time to exit.
  timeToExit = true;

  // Wait for thread to terminate.
  pthread_join(eventConsumerThread,0);

  // We're done with the receiver.
  tearDownReceiver();

  //-------------------------------------
  // Release resources in this order.
  //-------------------------------------
  if (dataConsumerPtr != 0)
  {
    delete dataConsumerPtr;
  } // if

  if (receiveDataProcessorPtr != 0)
  {
    delete receiveDataProcessorPtr;
  } // if

  if (amDemodulatorPtr != 0)
  {
    delete amDemodulatorPtr;
  } // if

  if (fmDemodulatorPtr != 0)
  {
    delete fmDemodulatorPtr;
  } // if

  if (wbFmDemodulatorPtr != 0)
  {
    delete wbFmDemodulatorPtr;
  } // if

  if (ssbDemodulatorPtr != 0)
  {
    delete ssbDemodulatorPtr;
  } // if

  if (receiveBufferPtr != 0)
  {
    free(receiveBufferPtr);
  } // if

  if (agcDisplayBufferPtr != 0)
  {
    delete[] agcDisplayBufferPtr;
  } // if
  //-------------------------------------

  // Destroy radio lock.
  pthread_mutex_destroy(&radioLock);

  // Destroy the I/O subsystem lock.
  pthread_mutex_destroy(&ioSubsystemLock);

  return;
 
} // ~Radio

/**************************************************************************

  Name: setupReceiver

  Purpose: The purpose of this function is to set up the receiver with
  a default configuration.

  Calling Sequence: success = setupReceiver()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setupReceiver(void)
{
  bool success;
  int error;

  // Default to success.
  success = true;

  // Indicate that the receive process is disabled.
  receiveEnabled = false;

  // Default to 100MHz.
  receiveFrequency = 100000000LL;

  // Use automatic bandwidth selection.
  receiveBandwidth = 0;

  // Default to automatic gain mode.
  receiveGainInDb = RADIO_RECEIVE_AUTO_GAIN;

  // Set to something sane.
  receiveIfGainInDb = 24;

  // Other subsystems need to know this.
  radio_adjustableReceiveGainInDb = receiveIfGainInDb;

  // Default to no frequency error.
  receiveWarpInPartsPerMillion = 0;

  return (success);
 
} // setupReceiver

/**************************************************************************

  Name: tearDownReceiver

  Purpose: The purpose of this function is to tear down the receiver.

  Calling Sequence: tearDownReceiver()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::tearDownReceiver(void)
{

  // Stop all data flow.
  stopReceiver();

  return;
  
} // tearDownReceiver

/**************************************************************************

  Name: startReceiver

  Purpose: The purpose of this function is to enable the receiver.  The
  underlying system software will enable the receive path for the RFICs
  of interest, configure the hardware to enable data transfer for the 
  receive path, and enable receive event generation.

  Calling Sequence: startReceiver()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::startReceiver(void)
{
  bool success;
  bool status;
  int deviceIndex;
  char deviceNumberString[20];
  int error;
  int errorCount;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to success
  success = true;

  // Indicate that no errors occurred.
  errorCount = 0;

  if (!receiveEnabled)
  {
    // Format our parameter.
    snprintf(deviceNumberString,sizeof(deviceNumberString),"%d",deviceNumber);

    // See if our radio device is available.
    deviceIndex = verbose_device_search(deviceNumberString);

    if (deviceIndex >= 0)
    {
      // Start up the radio device.         
      error = rtlsdr_open((rtlsdr_dev_t **)&devicePtr,(uint32_t)deviceIndex);

      if (error == 0)
      {
        // Set the receive frequency.
        status = setReceiveFrequency(receiveFrequency);

        if (!status)
        {
          // Indicate that one more error occurred.
          errorCount++;
        } // if

        // Set the receive sample rate.
        status = setReceiveSampleRate(receiveSampleRate);

        if (!status)
        {
          // Indicate that one more error occurred.
          errorCount++;
        } // if

        // Set the receive bandwidth.
        status = setReceiveBandwidth(receiveBandwidth);

        if (!status)
        {
          // Indicate that one more error occurred.
          errorCount++;
        } // if

        // We need to do this since there is no VGA..
        status =  setReceiveIfGainInDb(0,receiveIfGainInDb);

        if (!status)
        {
          // Indicate that one more error occurred.
          errorCount++;
        } // if

        // Set the receive gain.
        status = setReceiveGainInDb(receiveGainInDb);

        if (!status)
        {
          // Indicate that one more error occurred.
          errorCount++;
        } // if

        // Set the ppm error.
        status = setReceiveWarpInPartsPerMillion(receiveWarpInPartsPerMillion);

        if (!status)
        {
          // Indicate that one more error occurred.
          errorCount++;
        } // if

        if (errorCount == 0)
        {  
          // Ensure that the data consumer will accept data.
          dataConsumerPtr->start();

          // Reset the endpoint to ensure that data stream is synchronized.
          verbose_reset_buffer((rtlsdr_dev_t *)devicePtr);

          // Ensure that the system will accept any arriving data.
          receiveEnabled = true;
        } // if
        else
        {
          // Errors occurred, so let's close the radio.
          error = rtlsdr_close((rtlsdr_dev_t *)devicePtr);

          // Indicate that the radio hardware is not available.
          devicePtr = 0;

          // Indicate failure.
          success = false;
        } // else
      } // if
      else
      {
        // Indicate failure.
        success = false;
      } // else
    } // if
    else
    {
      // Indicate failure.
      success = false;
    } // else
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // startReceiver

/**************************************************************************

  Name: stopReceiver

  Purpose: The purpose of this function is to disable the receiver.  The
  underlying system software will disable the receive path for the RFICs
  of interest, configure the hardware to disable DMA for the receive path,
  and disable receive event generation.

  Calling Sequence: stopReceiver()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::stopReceiver(void)
{

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  if (receiveEnabled)
  {
    // Request to stop the receiver.
    requestReceiverToStop = true;

    // Acquire the radio lock.
    pthread_mutex_lock(&radioLock);

    // Ensure that the system will discard any arriving data.
    receiveEnabled = false;

    // Ensure that the data consumer will not accept data.
    dataConsumerPtr->stop();

    // Stop the radio.
    rtlsdr_close((rtlsdr_dev_t *)devicePtr);

    // We don't like dangling pointers.
    devicePtr = 0;

    // The radio is stopped, so we're done..
    requestReceiverToStop = false;

    // Disable the AGC to avoid startup transients later.
    agc_disable();

    // Release the radio lock.
    pthread_mutex_unlock(&radioLock);
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return;
  
} // stopReceiver

/**************************************************************************

  Name: setReceiveFrequency

  Purpose: The purpose of this function is to set the operating frequency
  of the receiver.  If the receiver is not enabled, the appropriate
  attribute is updated.  The attribute will be used at a later time to
  set up the receiver.
  Note that the definition of receiver enabled is that devicePtr is not
  equal to zero.

  Calling Sequence: success = setReceiveFrequency(frequency)

  Inputs:

    frequency - The receiver frequency in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveFrequency(uint64_t frequency)
{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Notify the driver of the new frequency.
    error = rtlsdr_set_center_freq((rtlsdr_dev_t *)devicePtr,
                                   (uint32_t)frequency + receiveSampleRate / 4);

    if (error == 0)
    {
      // Update attribute.
      receiveFrequency = frequency;

      // indicate success.
      success = true;
    } // if
  } // if
  else
  {
    // Update attribute.
    receiveFrequency = frequency;

    // indicate success.
    success = true;
  } // else

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveFrequency

/**************************************************************************

  Name: setReceiveBandwidth

  Purpose: The purpose of this function is to set the baseband filter
  bandwidth of the receiver.  If the receiver is not enabled, the
  appropriate attribute is updated.  The attribute will be used at a
  later time to set up the receiver.
  Note that the definition of receiver enabled is that devicePtr is not
  equal to zero.

  Calling Sequence: success = setReceiveBandwidth(bandwidth)

  Inputs:

    bandwidth - The receive baseband filter bandwidth in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveBandwidth(uint32_t bandwidth)
{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Notify the driver of the new bandwidth.
    error = rtlsdr_set_tuner_bandwidth((rtlsdr_dev_t *)devicePtr,bandwidth);

    if (error == 0)
    {
      // Update attribute.
      receiveBandwidth = bandwidth;

      // indicate success.
      success = true;
    } // if
  } // if
  else
  {
    // Update attribute.
    receiveBandwidth = bandwidth;

    // indicate success.
    success = true;
  } // else

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveBandwidth

/**************************************************************************

  Name: setReceiveGainInDb

  Purpose: The purpose of this function is to set the gain of the
  receiver.  If the receiver is not enabled, the appropriate attribute is
  updated.  The attribute will be used at a later time to set up the
  receiver.
  Note that the definition of receiver enabled is that devicePtr is not
  equal to zero.

  Calling Sequence: success = setReceiveGainInDb(gain)

  Inputs:

    gain - The gain in decibels.  A value of 0xffffffff indicates that
    the system should be set to automatic gain mode.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveGainInDb(uint32_t gain)

{
  bool success;
  int nearestGain;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Notify the driver of the new gain.
    if (gain == RADIO_RECEIVE_AUTO_GAIN)
    {
      // Set to automatic gain mode.
      error = verbose_auto_gain((rtlsdr_dev_t *)devicePtr);
    } // if
    else
    {
      // Retrieve the nearest gain value in 0.1 decibel.
      nearestGain = nearest_gain((rtlsdr_dev_t *)devicePtr,(gain * 10));

      // Set the gain.
      error = verbose_gain_set((rtlsdr_dev_t *)devicePtr,nearestGain);
    } // else

    if (error == 0)
    {
      // Update attribute.
      receiveGainInDb = gain;

      // indicate success.
      success = true;
    } // if
  } // if
  else
  {
    // Update attribute.
    receiveGainInDb = gain;

    // indicate success.
    success = true;
  } // else

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveGainInDb

/**************************************************************************

  Name: setReceiveIfGainInDb

  Purpose: The purpose of this function is to set the If gain of the
  receiver.  If the receiver is not enabled, the appropriate attribute is
  updated.  The attribute will be used at a later time to set up the
  receiver.
  Note that the definition of receiver enabled is that devicePtr is not
  equal to zero.

  Calling Sequence: success = setReceiveIfGainInDb(stage,gain)

  Inputs:

    stage - The amplifier stage for which the IF gain is to be
    modified.

    gain - The gain in decibels.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveIfGainInDb(uint8_t stage,uint32_t gain)
{
  bool success;
  int nearestGain;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  // Kludge to honor the librtlsdr interface.
  nearestGain = (int)gain;

  if (devicePtr != 0)
  {
    // Set the gain.
    error = rtlsdr_set_tuner_if_gain((rtlsdr_dev_t *)devicePtr,
                                     stage,
                                     (gain * 10));

    if (error == 0)
    {
      // Update attribute.
      receiveIfGainInDb = gain;

      // This variable is used by other subsystems.
      radio_adjustableReceiveGainInDb = receiveIfGainInDb;

      // indicate success.
      success = true;
    } // if
  } // if
  else
  {
    // Update attribute.
    receiveIfGainInDb = gain;

    // This variable is used by other subsystems.
    radio_adjustableReceiveGainInDb = receiveIfGainInDb;

    // indicate success.
    success = true;
  } // else

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveIfGainInDb

/**************************************************************************

  Name: setReceiveSampleRate

  Purpose: The purpose of this function is to set the sample rate
  of the receiver.  If the receiver is not enabled, the appropriate
  attribute is updated.  The attribute will be used at a later time to set
  up the receiver.
  Note that the definition of receiver enabled is that devicePtr is not
  equal to zero.

  Calling Sequence: success = setReceiveSampleRate(sampleRate)

  Inputs:

    sampleRate - The receiver sample rate in samples per second.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveSampleRate(uint32_t sampleRate)
{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Notify the driver of the new sample rate.
    error = verbose_set_sample_rate((rtlsdr_dev_t *)devicePtr,sampleRate);

    if (error == 0)
    {
      // Update attribute.
      receiveSampleRate = sampleRate;

      // indicate success.
      success = true;
    } // if
  } // if
  else
  {
    // Update attribute.
    receiveSampleRate = sampleRate;

    // indicate success.
    success = true;
  } // else

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveSampleRate

/**************************************************************************

  Name: setReceiveWarpInPartsPerMillion

  Purpose: The purpose of this function is to set the frequency warp
  of the receiver.  If the receiver is not enabled, the appropriate
  attribute is updated.  The attribute will be used at a later time to set
  up the receiver.
  Note that the definition of receiver enabled is that devicePtr is not
  equal to zero.

  Calling Sequence: success = setReceiveWarpInPartsPerMillion(warp)

  Inputs:

    warp - The warp value in parts per million.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveWarpInPartsPerMillion(int warp)
{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Notify the driver of the new warp value.
    error = verbose_ppm_set((rtlsdr_dev_t *)devicePtr,warp);

    if (error == 0)
    {
      // Update attribute.
      receiveWarpInPartsPerMillion = warp;

      // indicate success.
      success = true;
    } // if
  } // if
  else
  {
    // Update attribute.
    receiveWarpInPartsPerMillion = warp;

    // indicate success.
    success = true;
  } // else

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveWarpInPartsPerMillion

/**************************************************************************

  Name: enableIqDump

  Purpose: The purpose of this function is to enable the streaming of
  IQ data over a UDP connection.  This allows a link parter to
  process this data in any required way: for example, demodulation,
  spectrum analysis, etc.

  Calling Sequence: enableIqDump()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::enableIqDump(void)
{

  // Enable the streaming of IQ data over a UDP connection.
  receiveDataProcessorPtr->enableIqDump();

  return;

} // enableIqDump

/**************************************************************************

  Name: disableIqDump

  Purpose: The purpose of this function is to disable the streaming of
  IQ data over a UDP connection.
  Calling Sequence: disableIqDump()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::disableIqDump(void)
{

  // Disable the streaming of IQ data over a UDP connection.
  receiveDataProcessorPtr->disableIqDump();

  return;

} // disableIqDump

/**************************************************************************

  Name: isIqDumpEnabled

  Purpose: The purpose of this function is to determine whether or not
  the dumping of IQ data, over a UDP connection is enabled or not.

  Calling Sequence: enabled = isIqDumpEnabled()

  Inputs:

    None.

  Outputs:

    enabled - A flag that indicates whether or not the dumping of IQ
    data is enabled.  A value of true indicates that IQ dumping is
    enabled, and a value of false indicates that IQ dumping is disabled.

**************************************************************************/
bool Radio::isIqDumpEnabled(void)
{
  bool enabled;

  // Retrieve the IQ dump state.
  enabled = receiveDataProcessorPtr->isIqDumpEnabled();

  return (enabled);

} // isIqDumpEnabled

/**************************************************************************

  Name: setSignalDetectThreshold

  Purpose: The purpose of this function is to set the signal detect
  threshold.  A signal is considered as detected if a signal level,
  in dBFs, matches or exceeds the threshold value.

  Calling Sequence: success = setSignalDetectThreshold(threshold)

  Inputs:

    threshold - The signal detection threshold.  Valid values are,
    -100 <= threshold <= 10.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setSignalDetectThreshold(int32_t threshold)
{
  bool success;

  // Default to failure.
  success = false;

  if ((threshold >= (-400)) && (threshold <= 10))
  {
    // Inform data processor of new threshold.
    receiveDataProcessorPtr->setSignalDetectThreshold(threshold);

    // Indicate success.
    success = true;
  } // if

  return (success);

} // setSignalDetectThreshold

/**************************************************************************

  Name: getReceiveFrequency

  Purpose: The purpose of this function is to get the operating frequency
  of the receiver.

  Calling Sequence: frequency = getReceiveFrequency()

  Inputs:

    None.

  Outputs:

    frequency - The receiver frequency in Hertz.

**************************************************************************/
uint64_t Radio::getReceiveFrequency(void)
{

  return (receiveFrequency);
  
} // getReceiveFrequency

/**************************************************************************

  Name: getReceiveBandwidth

  Purpose: The purpose of this function is to get the baseband filter
  bandwidth of the receiver.

  Calling Sequence: bandwidth = setReceiveBandwidth()

  Inputs:

    None.

  Outputs:

    bandwidth - The receive baseband filter bandwidth in Hertz.

**************************************************************************/
uint32_t Radio::getReceiveBandwidth(void)
{
  return (receiveBandwidth);
  
} // getReceiveBandwidth

/**************************************************************************

  Name: getReceiveGainInDb

  Purpose: The purpose of this function is to get the gain of the
  receiver.

  Calling Sequence: gain = getReceiveGainInDb()

  Inputs:

    None.

  Outputs:

    gain - The gain in decibels.

**************************************************************************/
uint32_t Radio::getReceiveGainInDb(void)

{

  return (receiveGainInDb);
  
} // getReceiveGainInDb

/**************************************************************************

  Name: getReceiveIfGainInDb

  Purpose: The purpose of this function is to get the IF gain of the
  receiver.

  Calling Sequence: gain = getReceiveIfGainInDb()

  Inputs:

    None.

  Outputs:

    gain - The gain in decibels.

**************************************************************************/
uint32_t Radio::getReceiveIfGainInDb(void)

{

  return (receiveIfGainInDb);
  
} // getReceiveIfGainInDb

/**************************************************************************

  Name: getReceiveSampleRate

  Purpose: The purpose of this function is to get the sample rate
  of the receiver.

  Calling Sequence: sampleRate = getReceiveSampleRate()

  Inputs:

    None.

  Outputs:

    sampleRate - The receiver sample rate in samples per second.

**************************************************************************/
uint32_t Radio::getReceiveSampleRate(void)
{

  return (receiveSampleRate);
  
} // getReceiveSampleRate

/**************************************************************************

  Name: getReceiveWarpInPartsPerMillion

  Purpose: The purpose of this function is to get the frequency warp value
  of the receiver.

  Calling Sequence: warp = getReceiveWarpInPartsPerMillion()

  Inputs:

    None.

  Outputs:

    warp - The frequency warp value in parts per million.

**************************************************************************/
int Radio::getReceiveWarpInPartsPerMillion(void)
{

  return (receiveWarpInPartsPerMillion);
  
} // getReceiveWarpInPartsPerMillion

/**************************************************************************

  Name: isReceiving

  Purpose: The purpose of this function is to indicate whether or not the
  receiver is enabled.

  Calling Sequence: enabled = isReceiving()

  Inputs:

    None.

  Outputs:

    Enabled - A boolean that indicates whether or not the receiver is
    enabled.  A value of true indicates that the receiver is enabled,
    and a value of false indicates that the receiveer is disabled.

**************************************************************************/
bool  Radio::isReceiving(void)
{

  return (receiveEnabled);
  
} // isReceiving

/**************************************************************************

  Name: setDemodulatorMode

  Purpose: The purpose of this function is to set the demodulator mode.
  This mode dictates which demodulator should be used when processing IQ
  data samples.

  Calling Sequence: setDemodulatorMode(mode)

  Inputs:

    mode - The demodulator mode.  Valid values are None, Am, and Fm.

  Outputs:

    None.

**************************************************************************/
void Radio::setDemodulatorMode(uint8_t mode)
{
  IqDataProcessor::demodulatorType kludge;

  // Let's make things happy.
  kludge = static_cast<IqDataProcessor::demodulatorType>(mode);

  // Notify the IQ data processor to use this mode.
  receiveDataProcessorPtr->setDemodulatorMode(kludge);

  return;

} // setDemodulaterMode

/*****************************************************************************

  Name: setAmDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the
  AM demodulator.

  Calling Sequence: setAmDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setAmDemodulatorGain(float gain)
{

  // Notifier the AM demodulator to set its gain.
  amDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setAmDemodulatorGain

/*****************************************************************************

  Name: setFmDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the
  FM demodulator.

  Calling Sequence: setFmDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setFmDemodulatorGain(float gain)
{

  // Notifier the FM demodulator to set its gain.
  fmDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setFmDemodulatorGain

/*****************************************************************************

  Name: setWbFmDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the wideband
  FM demodulator.

  Calling Sequence: setWbFmDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setWbFmDemodulatorGain(float gain)
{

  // Notifier the FM demodulator to set its gain.
  wbFmDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setWbFmDemodulatorGain

/*****************************************************************************

  Name: setSsbDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the SSB
  demodulator.

  Calling Sequence: setSsbDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setSsbDemodulatorGain(float gain)
{

  // Notifier the SSB demodulator to set its gain.
  ssbDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setSsbDemodulatorGain

/*****************************************************************************

  Name: getIqProcessor

  Purpose: The purpose of this function is to retrieve the address of the
  IqDataProcessor object.

  Calling Sequence: dataProcessorPtr = getIqProcessor()

  Inputs:

    None.

  Outputs:

    dataProcessorPtr - A pointer to an instance of an IqDataProcessor
    that is used by the Radio.

*****************************************************************************/
IqDataProcessor * Radio::getIqProcessor(void)
{

  return (receiveDataProcessorPtr);

} // getIqProcessorPtr

/**************************************************************************

  Name: setAgcType

  Purpose: The purpose of this function is to set the type of AGC
  algorithm to be used.

  Calling Sequence: success = setAgcType(type)

  Inputs:

    type - The type of AGC.  A value of 0 indicates that the lowpass AGC
    is to be used, and a value of 1 indicates that the Harris AGC is to
    be used.

  Outputs:

    success - A flag that indicates whether the the AGC type was set.
    A value of true indicates that the AGC type was successfully set,
    and a value of false indicates that the AGC type was not set due
    to an invalid value for the AGC type was specified.

**************************************************************************/
bool Radio::setAgcType(uint32_t type)
{
  bool success;

  success = true;

  return (success);

} // setAgcType

/**************************************************************************

  Name: setAgcDeadband

  Purpose: The purpose of this function is to set the deadband of the
  AGC.  This presents gain setting oscillations

  Calling Sequence: success = uint32_t deadband(deadbandInDb)

  Inputs:

    deadbandInDb - The deadband, in decibels, used to prevent unwanted
    gain setting oscillations.  A window is created such that a gain
    adjustment will not occur if the current gain is within the window
    new gain <= current gain += deadband.

  Outputs:

    success - A flag that indicates whether or not the deadband parameter
    was updated.  A value of true indicates that the deadband was
    updated, and a value of false indicates that the parameter was not
    updated due to an invalid specified deadband value.

**************************************************************************/
bool  Radio::setAgcDeadband(uint32_t deadbandInDb)
{
  bool success;

  // Update the AGC deadband.
  success = agc_setDeadband(deadbandInDb);

  return (success);

} // setAgcDeadband

/**************************************************************************

  Name: setBlankingLimit

  Purpose: The purpose of this function is to set the blanking limit of
  the AGC.  This presents gain setting oscillations.  What happens if
  transients exist after an adjustment is made is that the AGC becomes
  blind to the actual signal that is being received since the transient
  can swamp the system.

  Calling Sequence: success = setBlankingLimit(blankingLimit)

  Inputs:

    blankingLimit - The number of measurements to ignore before making
    the next gain adjustment.

  Outputs:

    success - A flag that indicates whether or not the blanking limit
    parameter was updated.  A value of true indicates that the blanking
    limit was updated, and a value of false indicates that the parameter
    was not updated due to an invalid specified blanking limit value.

**************************************************************************/
bool  Radio::setAgcBlankingLimit(uint32_t blankingLimit)
{
  bool success;

  // Set the blank counter limit to avoid transients.
  success = agc_setBlankingLimit(blankingLimit);

  return (success);

} // setAgcBlankingLimit

/**************************************************************************

  Name: setAgcOperatingPoint

  Purpose: The purpose of this function is to set the operating point
  of the AGC.

  Calling Sequence: setAgcOperatingPoint(operatingPointInDbFs)
  Inputs:

    operatingPointInDbFs - The operating point in decibels referenced to
    the full scale value.

  Outputs:

    None.

**************************************************************************/
void Radio::setAgcOperatingPoint(int32_t operatingPointInDbFs)
{

  // Update operating point.
  agc_setOperatingPoint(operatingPointInDbFs);

  return;

} // setAgcOperatingPoint

/**************************************************************************

  Name: setAgcFilterCoefficient

  Purpose: The purpose of this function is to set the coefficient of
  the first order lowpass filter that filters the baseband gain value.
  In effect, the time constant of the filter is set.

  Calling Sequence: success = setAgcFilterCoefficient(coefficient)

  Inputs:

    coefficient - The filter coefficient for the lowpass filter that
    // filters the baseband gain value.

  Outputs:

    success - A flag that indicates whether the filter coefficient was
    updated.  A value of true indicates that the coefficient was
    updated, and a value of false indicates that the coefficient was
    not updated due to an invalid coefficient value.

**************************************************************************/
bool  Radio::setAgcFilterCoefficient(float coefficient)
{
  bool success;

  // Update the lowpass filter coefficient.
  success = agc_setAgcFilterCoefficient(coefficient);

  return (success);

} // setAgcFilterCoefficient

/**************************************************************************

  Name: enableAgc

  Purpose: The purpose of this function is to enable the AGC.

  Calling Sequence: success = enableAgc();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the AGC was already
    enabled.

**************************************************************************/
bool Radio::enableAgc(void)
{
  bool success;

  // Enable the AGC.
  success = agc_enable();

  return (success);

} // enableAgc

/**************************************************************************

  Name: disableAgc

  Purpose: The purpose of this function is to disable the AGC.

  Calling Sequence: success = disableAgc();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the AGC was already
    disabled.

**************************************************************************/
bool Radio::disableAgc(void)
{
  bool success;

  // Disable the AGC.
  success = agc_disable();

  return (success);

} // disableAgc

/**************************************************************************

  Name: isAgcEnabled

  Purpose: The purpose of this function is to determine whether or not
  the AGC is enabled.

  Calling Sequence: status = isAgcEnabled()

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the AGC is enabled.
    A value of true indicates that the AGC is enabled, and a value of
    false indicates that the AGC is disabled.

**************************************************************************/
bool Radio::isAgcEnabled(void)
{

  return (agc_isEnabled());

} // isAgcEnabled

/**************************************************************************

  Name: displayAgcInternalInformation

  Purpose: The purpose of this function is to display information in the
  AGC.

  Calling Sequence: displayAgcInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::displayAgcInternalInformation(void)
{

  agc_displayInternalInformation(&agcDisplayBufferPtr);
  nprintf(stderr,"%s",agcDisplayBufferPtr);

  return;

} // displayAgcInternalInformation

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal
  in the radio.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::displayInternalInformation(void)
{

  // Display top level radio information.
  nprintf(stderr,"\n------------------------------------------------------\n");
  nprintf(stderr,"Radio Internal Information\n");
  nprintf(stderr,"------------------------------------------------------\n");

  nprintf(stderr,"Receive Enabled                     : ");
  if (receiveEnabled)
  {
    nprintf(stderr,"Yes\n");
  } // if
  else
  {
     nprintf(stderr,"No\n");
   } // else

  if (receiveGainInDb == RADIO_RECEIVE_AUTO_GAIN)
  {
    nprintf(stderr,"Receive Gain:                       : Auto\n");
  } // if
  else
  {
    nprintf(stderr,"Receive Gain:                       : %u dB\n",
          receiveGainInDb);
  } // else

  if (agc_isEnabled())
  {
    nprintf(stderr,"Receive IF Gain                     : Auto\n");
  } // if
  else
  {
    nprintf(stderr,"Receive IF Gain                     : %u dB\n",
            receiveIfGainInDb);
  } // else

  nprintf(stderr,"Receive Frequency                   : %llu Hz\n",
          receiveFrequency);
  nprintf(stderr,"Receive Bandwidth                   : %u Hz\n",
          receiveBandwidth);
  nprintf(stderr,"Receive Sample Rate:                : %u S/s\n",
          receiveSampleRate);
  nprintf(stderr,"Receive Frequency Warp:             : %d ppm\n",
          receiveWarpInPartsPerMillion);
  nprintf(stderr,"Receive Timestamp                   : %u\n",
          receiveTimeStamp);
  nprintf(stderr,"Receive Block Count                 : %u\n",
          receiveBlockCount);

  // Display data consumer information.
  dataConsumerPtr->displayInternalInformation();

  // Display IQ data processor information.
  receiveDataProcessorPtr->displayInternalInformation();

  // Display AM demodulator information to the user.
  amDemodulatorPtr->displayInternalInformation();

  // Display FM demodulator information to the user.
  fmDemodulatorPtr->displayInternalInformation();

  // Display wideband FM demodulator information to the user.
  wbFmDemodulatorPtr->displayInternalInformation();

  // Display wideband SSB demodulator information to the user.
  ssbDemodulatorPtr->displayInternalInformation();

  return;

} // displayInternalInformation

/*****************************************************************************

  Name: eventConsumerProcedure

  Purpose: The purpose of this function is to handle event notification
  functionality of the system.

  Calling Sequence: eventConsumerProcedure(arg)

  Inputs:

    arg - A pointer to an argument.

  Outputs:

    None.

*****************************************************************************/
void Radio::eventConsumerProcedure(void *arg)
{
  Radio *mePtr;
  int numberOfBytesRead;
  int error;

  fprintf(stderr,"Entering Radio Event Consumer.\n");

  // Save the pointer to the object.
  mePtr = (Radio *)arg;

  // Allow this thread to run.
  mePtr->timeToExit = false;

  while (!mePtr->timeToExit)
  {
    if (mePtr->receiveEnabled)
    {
      if (!mePtr->requestReceiverToStop)
      {
        // Acquire the radio lock.
        pthread_mutex_lock(&mePtr->radioLock);

        // Perform what is needed to read IQ samples.
        error = rtlsdr_read_sync((rtlsdr_dev_t *)mePtr->devicePtr,
                                 mePtr->receiveBufferPtr, 
                                 RECEIVE_BUFFER_SIZE,
                                 &numberOfBytesRead);

        // Indicate that more samples have been received.
        mePtr->receiveTimeStamp += (uint32_t )(numberOfBytesRead >> 1);

        // Send the IQ data to the data consumer system.
        mePtr->dataConsumerPtr->acceptData(mePtr->receiveTimeStamp,
                                           mePtr->receiveBufferPtr,
                                           (uint32_t)numberOfBytesRead);

        // Indicate that one more block of data has been received.
        mePtr->receiveBlockCount++;

        // Release the radio lock.
        pthread_mutex_unlock(&mePtr->radioLock);
      } // if
    } // if
    else
    {
      // Be nice to the system.
      usleep(5000);
    } // else
  } // while */

  fprintf(stderr,"Exiting Radio Event Consumer.\n");

  pthread_exit(0);

} // eventConsumerProcedure

