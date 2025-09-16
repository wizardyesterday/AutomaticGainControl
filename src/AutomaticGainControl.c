//**********************************************************************
// file name: AutomaticGainControl.c
//**********************************************************************

#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "AutomaticGainControl.h"

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Hardware-dependent defines.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
#define MAX_ADJUSTIBLE_GAIN (46)
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

// All private stuff is bundled in one structure.
static struct privateData
{
  // Don't run unless the system has been initialized.
  int initialized;

  // These parameters are sometimes needed to avoid transients.
  uint32_t blankingCounter;
  uint32_t blankingLimit;
  int gainWasAdjusted;

  // Yes, we need some deadband.
  int32_t deadbandInDb;

  // If 1, the AGC is running.
  int enabled;

  int32_t signalInDbFs;

  // The goal.
  int32_t operatingPointInDbFs;

  // AGC lowpass filter coefficient for baseband gain filtering.
  float alpha;

  // System gains.
  uint32_t gainInDb;
 
  // Filtered gain.
  float filteredGainInDb;

  // Signal level before amplification.
  int32_t normalizedSignalLevelInDbFs;

  // Gain set callback pointer to request client to set gain.
  void (*setGainCallbackPtr)(uint32_t gainIndB);

  // Gain rerieval callback pointer to request gain from e client.
  uint32_t (*getGainCallbackPtr)(void);
} me;

static void resetBlankingSystem(void);
static void run(int32_t signalIndBFs);
static void runHarris(int32_t signalIndBFs);

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Hardware-dependent functions to be filled in.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
static void setHardwareGainInDb(uint32_t gainInDb);
static uint32_t getHardwareGainInDb(void);
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Hardware-pecific functions.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

/**************************************************************************

  Name: setHardwareGainInDb

  Purpose: The purpose of this function is to set the gain of the
  variable gain amplifier in the hardware.
  It is the responsibility of the callback function to set the hardware
  gain since the user application is the entity that actually sets the
  gain.

  Calling Sequence: setHardwareGainInDb(gainInDb)

  Inputs:

    gain - The gain in decibels.

  Outputs:

    None.

**************************************************************************/
void setHardwareGainInDb(uint32_t gainInDb)
{

  // The client callback will perform hardware-centric processing.
  if (me.setGainCallbackPtr != 0)
  {
    me.setGainCallbackPtr(gainInDb);
  } // if

  return;

} // setHardwareGainInDb

/**************************************************************************

  Name: getHardwareGainInDb

  Purpose: The purpose of this function is the interface to run the AGC.
  It is the responsibility of the developer, that uses this AGC, to 
  supply a callback function for retrieving te current gain if the client
  had made any gain changes. In my opioion, the AGC should be disabled if
  the user wants to manually change the gain.  The only reason this
  function is needed is to avoid inconnsistancy between the gain that the
  user manually set and what the AGC automatically set.

  Calling Sequence: gainInDb = getHardwareGainInDb()

  Inputs:

    signalIndB- The gain in decibels.

  Outputs:

    None.

**************************************************************************/
uint32_t getHardwareGainInDb(void)
{
  uint32_t gainInDb;

  // Default if we don't have a cient callback function.
  gainInDb = me.gainInDb;

 // The client callback will perform hardware-centric processing.
  if (me.getGainCallbackPtr != 0)
  {
    gainInDb = me.getGainCallbackPtr();

    if (gainInDb <= MAX_ADJUSTIBLE_GAIN)
    {
      // The gain is in range.
      gainInDb = me.gainInDb;
    } // if
  } // if
 
  return (gainInDb);

} // getHardwareGainInDb

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// End of hardware-specific functions
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

/**************************************************************************

  Name: agc_acceptData

  Purpose: The purpose of this function is the interface to run the AGC.

  Calling Sequence: agc_acceptData(int32_t signalIndBFs)

  Inputs:

    signalIndBFs - The signal in decibels referenced to full scale.

  Outputs:

    None.

**************************************************************************/
void agc_acceptData(int32_t signalIndBFs)
{

  // Allow the AGC to poerate if it is configured.
  if (me.initialized)
  {
    if (me.enabled)
    {
      // Process the signal.
      run(signalIndBFs);
    } // if
  } // if

  return;

} // agc_acceptData

/************************************************************************

  Name: agc_init

  Purpose: The purpose of this function is to initialize the AGC
  subsystem.

  Calling Sequence: agc_init(operatingPointInDbFs)

  Inputs:

    operatingPointInDbFs - The AGC operating point in decibels referenced
    to full scale.  Full scale represents 0dBFs, otherwise, all other
    values will be negative.

    setGainCallbackPtr - A pointer to a client callback function that
    is invoked to request the client application to set the hardware gain.
    A value of NULL is used if the client does not want to register a
    callback.
    If a callback is not registered,the AGC will never modify modify
    the hardware gain.  In other words, rhe system will have no AGC.

    getGainCallbackPtr - A pointer to a client callback function that
    is invoked to request the client application to supply the hardware
    gain.
    A value of NULL is used if the client does not want to register a
    callback.
    In  other words, if a callback is not registered, the AGC will have
    no knowledge if the gain was modifies by anyone  else. DO THIS AT
    YOUR OWN RISK.

  Outputs:

    None.

**************************************************************************/
void agc_init(int32_t operatingPointInDbFs,
    void (*setGainCallbackPtr)(uint32_t gainIndB),
    uint32_t (*getGainCallbackPtr)(void))
{

  // Reference the set point to the antenna input.
  me.operatingPointInDbFs = operatingPointInDbFs;

  // Start with a reasonable deadband.
  me.deadbandInDb = 1;

  // Set this to the midrange.
  me.signalInDbFs = -12;

  // Default to disabled.
  me.enabled = 0;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Sometimes, adjustments need to be avoided when a transient in the
  // hardware occurs as a result of a gain adjustment.  In the rtlsdr,
  // it was initially thought that the tagc_init(int32_t operatingPointInDbFs)ransient was occurring in the
  // tuner chip.  This was not the case.  Instead, a transient in the
  // demodulated data was occurring as a result of the IIC repeater
  // being enabled (and/or disabled) in the Realtek 2832U chip.  The
  // simplest thing to do in software is to perform a transient
  // avoidance strategy.  While it is 1 that the performance of the
  // AGC becomes less than optimal, it is still better than experiencing
  // limit cycles.  The blankingLimit is configurable so that the
  // user can change the value to suit the needs of the application.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  me.blankingCounter = 0;
  me.blankingLimit = 1;

  // Allow the AGC to run the first time.
  me.gainWasAdjusted = 0;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 // This is a good starting point for the receiver gain
  // values.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  me.gainInDb = 24;

  me.normalizedSignalLevelInDbFs = -me.gainInDb;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // AGC filter initialization.  The filter is implemented
  // as a first-order difference equation.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Initial condition of filter memory.
  me.filteredGainInDb = 24;

  //+++++++++++++++++++++++++++++++++++++++++++++++++
  // Set the AGC time constant such that gain
  // adjustment occurs rapidly while maintaining
  // system stability.
  //+++++++++++++++++++++++++++++++++++++++++++++++++
  me.alpha = 0.8;
  //+++++++++++++++++++++++++++++++++++++++++++++++++
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Register the client request callbacks.
  me.setGainCallbackPtr = setGainCallbackPtr;
  me.getGainCallbackPtr = getGainCallbackPtr;

  // This is the top level qualifier for the AGC to run.
  me.initialized = 1;

  return;
 
} // agc_init

/**************************************************************************

  Name: agc_setDeadband

  Purpose: The purpose of this function is to set the deadband of the
  AGC.  This presents gain setting oscillations

  Calling Sequence: success = agc_setDeadband(deadbandInDb)

  Inputs:

    deadbandInDb - The deadband, in decibels, used to prevent unwanted
    gain setting oscillations.  A window is created such that a gain
    adjustment will not occur if the current gain is within the window
    new gain <= current gain += deadband.

  Outputs:

    success - A flag that indicates whether or not the deadband parameter
    was updated.  A value of 1 indicates that the deadband was
    updated, and a value of 0 indicates that the parameter was not
    updated due to an invalid specified deadband value.

**************************************************************************/
int agc_setDeadband(uint32_t deadbandInDb)
{
  int success;

  // Default to failure.
  success = 0;

  if ((deadbandInDb >= 0) && (deadbandInDb <= 10))
  {
    // Update the attribute.
    me.deadbandInDb = deadbandInDb;

    // Indicate success.
    success = 1;
  } // if

  return (success);

} // agc_setDeadband

/**************************************************************************

  Name: agc_setBlankingLimit

  Purpose: The purpose of this function is to set the blanking limit of
  the AGC.  This presents gain setting oscillations.  What happens if
  transients exist after an adjustment is made is that the AGC becomes
  blind to the actual signal that is being received since the transient
  can swamp the system.

  Calling Sequence: success = agc_setBlankingLimit(blankingLimit)

  Inputs:

    blankingLimit - The number of measurements to ignore before making
    the next gain adjustment.

  Outputs:

    success - A flag that indicates whether or not the blanking limit
    parameter was updated.  A value of 1 indicates that the blanking
    limit was updated, and a value of 0 indicates that the parameter
    was not updated due to an invalid specified blanking limit value.

**************************************************************************/
int agc_setBlankingLimit(uint32_t blankingLimit)
{
  int success;

  // Default to failure.
  success = 0;

  if ((blankingLimit >= 0) && (blankingLimit <= 10))
  {
    // Update the attributes.
    me.blankingLimit = blankingLimit;

    // Set the blanking system to its initial state.
    resetBlankingSystem();

    // Indicate success.
    success = 1;
  } // if

  return (success);

} // agc_setBlankingLimit

/**************************************************************************

  Name: setOperatingPoint

  Purpose: The purpose of this function is to set the operating point
  of the AGC.

  Calling Sequence: agc_setOperatingPoint(operatingPointInDbFs)

  Inputs:

    operatingPointInDbFs - The operating point in decibels referenced to
    the full scale value.

  Outputs:

    None.

**************************************************************************/
void agc_setOperatingPoint(int32_t operatingPointInDbFs)
{

  // Update operating point.
  me.operatingPointInDbFs = operatingPointInDbFs;

  return;

} // agc_setOperatingPoint

/**************************************************************************

  Name: agc_setAgcFilterCoefficient

  Purpose: The purpose of this function is to set the coefficient of
  the first order lowpass filter that filters the baseband gain value.
  In effect, the time constant of the filter is set.

  Calling Sequence: success = agc_setAgcFilterCoefficient(coefficient)

  Inputs:

    coefficient - The filter coefficient for the lowpass filter that
    filters the baseband gain value.

  Outputs:

    success - A flag that indicates whether the filter coefficient was
    updated.  A value of 1 indicates that the coefficient was
    updated, and a value of 0 indicates that the coefficient was
    not updated due to an invalid coefficient value.

**************************************************************************/
int agc_setAgcFilterCoefficient(float coefficient)
{
  int success;

  // Default to failure.
  success = 0;

  if ((coefficient >= 0.001) && (coefficient < 0.999))
  {
    // Update the attribute.
    me.alpha = coefficient;

    // Indicate success.
    success = 1;
  } // if

  return (success);

} // agc_agc_setAgcFilterCoefficient

/**************************************************************************

  Name: agc_enable

  Purpose: The purpose of this function is to enable the AGC.

  Calling Sequence: success = agc_enable();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of 1 indicates that the operation was
    successful, and a value of 0 indicates that, eitherthe AGC
    was already enabled, or the radio was not in a receiving state.

**************************************************************************/
int agc_enable(void)
{
  int success;

 // Default to failure.
  success = 0;

  if (!me.enabled)
  {
    // Set the system to an initial state.
    resetBlankingSystem();

    // Enable the AGC.
    me.enabled = 1;

    // Indicate success.
    success = 1;
  } // if

  return (success);

} // agc_enable

/**************************************************************************

  Name: agc_disable

  Purpose: The purpose of this function is to disable the AGC.

  Calling Sequence: success = agc_disable();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of 1 indicates that the operation was
    successful, and a value of 0 indicates that the AGC was already
    disabled.

**************************************************************************/
int agc_disable(void)
{
  int success;

 // Default to failure.
  success = 0;

  if (me.enabled)
  {
    // Disable the AGC.
    me.enabled = 0;

    success = 1;
  } // if

  return (success);

} // agc_disable

/**************************************************************************

  Name: agc_isEnabled

  Purpose: The purpose of this function is to determine whether or not
  the AGC is enabled.

  Calling Sequence: status = agc_isEnabled()

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the AGC is enabled.
    A value of 1 indicates that the AGC is enabled, and a value of
    0 indicates that the AGC is disabled.

**************************************************************************/
int agc_isEnabled(void)
{

  return (me.enabled);

} // agc_isEnabled

/**************************************************************************

  Name: resetBlankingSystem

  Purpose: The purpose of this function is to reset the blanking system
  to its starting state.

  Calling Sequence: resetBlankingSystem()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void resetBlankingSystem(void)
{

  // Reset the counter for the next blanking interval.
  me.blankingCounter = 0;

  // Ensure that the AGC can run the next time.
  me.gainWasAdjusted = 0;

  return;

} // resetBlankingSystem

/**************************************************************************

  Name: run

  Purpose: The purpose of this function is to run the selected automatic
  gain control algorithm.
 
  Calling Sequence: run(int32_t signalIndBFs)

  Inputs:

    signalIndBFs) - The average magnitude of the IQ data block that
    was received in units of dBFs.

  Outputs:

    None.

**************************************************************************/
void run(int32_t signalIndBFs)
{
  int allowedToRun;
  uint32_t adjustableGain;

 // Default to not being able to run.
  allowedToRun = 0;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This block of code deals with the case where some external
  // entity adjusted the receiver IF gain without knowledge of
  // the AGC.  If this block of code were not here, data
  // inconsistancy can occur between the hardware setting of
  // the IF gain, and the AGC's idea of what the gain should be.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  adjustableGain = getHardwareGainInDb();

  if (me.gainInDb != adjustableGain)
  {
    me.gainInDb = adjustableGain;
  } // if
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This block of code ensures that if a gain adjustment was
  // made by the AGC, the system will blank itself for an
  // interval specified by the blankingLimit parameter.  If the
  // blankingLimit parameter is set to zero, blanking will be
  // disabled.  Note that if a gain adjustment was not made,
  // the AGC will be allowed to run.  This allows the AGC to
  // react quickly to signal changes.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if (me.gainWasAdjusted)
  {
    if (me.blankingCounter < me.blankingLimit)
    {
      // The systemn is still blanked.
      me.blankingCounter++;
    } // if
    else
    {
      // We're done blanking.
      resetBlankingSystem();

      // Let the AGC run.
      allowedToRun = 1;
    } // else
  } // if
  else
  {
    // Let the AGC run if no gain adjustment was made.

    allowedToRun = 1;
  } // else
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  if (allowedToRun)
  {
    runHarris(signalIndBFs);
  } // of

  return;

} // run

/**************************************************************************

  Name: runHarris

  Purpose: The purpose of this function is to run the automatic gain
  control.  This is the implementation of the algorithm described by
  Fredric Harris et. al. in the paper entitled, "On the Design,
  Implementation, and Performance of a Microprocessor-Controlled AGC
  System for a Digital Receiver".  This AGC is also described in
  "Understanding Digital Signal Processing, Third Edition", Section 13.30,
  "Automatic Gain Control (AGC)" by Richard G. Lyons (same AGC as described
  in the previously mentioned paper).

  Note:  A large gain error will result in a more rapid convergence
  to the operating point versus a small gain error.  Let the subsystem
  parameters be defined as follows:

    R - The reference point in dBFs.

    alpha - The system time constant.  This parameter dictates how
    quickly the AGC will converge to the reference point, R, as a
    function of the received signal magnitude (in dBFs). 

    x(n) - The received signal magnitude, in dBFs, referenced at the
    point before adjustable amplification.

    y(n) - The received signal that has been amplified by the adjustable
    gain amplifier in units of dBFs.

    e(n) - The deviation from the reference point in decibels.

    g(n) - The gain of the adjustable gain amplifier in decibels.

  The equations for the AGC algorithm are as follows.

    y(n) = x(n) + g(n).

    e(n) = R - y(n).

    g(n+1) = g(n) + [alpha * e(n)]


  Calling Sequence: runHarris(int32_t signalIndBFs)

  Inputs:

    signalIndBFs - The signal in decibels referenced to full scale.

  Outputs:

    None.

**************************************************************************/
void runHarris(int32_t signalIndBFs)
{
  int success;
  int32_t gainError;

  // Update for display purposes.
  me.normalizedSignalLevelInDbFs = signalIndBFs - me.gainInDb;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment.
  // Allocate gains appropriately.  Here is what we
  // have to work with:
  //
  //   1. The IF amp provides gains from 0 to 46dB
  //   in nonuniform steps.
  //
  // Let's try this first:
  //
  //   1. Adjust the gain as appropriate to achieve
  //   the operating point referenced at the antenna input.
  //
  // This provides a dynamic range of 46dB (since the IF
  // amplifier has an adjustable gain from 0 to 46dB), of the
  // samples that drive the A/D convertor.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment.
  gainError = me.operatingPointInDbFs - signalIndBFs;

  //**************************************************
  // Make sure that we aren't at the gain rails.  If
  // the system is already at maximum gain, and the
  // gain error is positive, we don't want to make an
  // adjustment.  Similarly, if the system is already
  // at minimum gain, and the gain error is negative,
  // we don't want to make an adjustment.  This is
  // easily solved by setting the gain error to zero.
  //************************************************** 
  if (me.gainInDb == MAX_ADJUSTIBLE_GAIN)
  {
    if (gainError > 0)
    {
      gainError = 0;
    } // if
  } // if
  else
  {
    if (me.gainInDb == 0)
    {
      if (gainError < 0)
      {
        gainError = 0;
      } // if
    } // if
  } // else
  //**************************************************

  // Apply deadband to eliminate gain oscillations.
  if (abs(gainError) <= me.deadbandInDb)
  {
    gainError = 0;
  } // if

  //*******************************************************************
  // Run the AGC algorithm.
  //*******************************************************************
  me.filteredGainInDb = me.filteredGainInDb +
    (me.alpha * (float)gainError);

  //+++++++++++++++++++++++++++++++++++++++++++
  // Limit the gain to valid values.
  //+++++++++++++++++++++++++++++++++++++++++++
  if (me.filteredGainInDb > MAX_ADJUSTIBLE_GAIN)
  {
    me.filteredGainInDb = MAX_ADJUSTIBLE_GAIN;
  } // if
  else
  {
    if (me.filteredGainInDb < 0)
    {
      me.filteredGainInDb = 0;
    } // if
  } // else
  //*******************************************************************

  // Update the attribute.
  me.gainInDb = (uint32_t)me.filteredGainInDb;

  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  // Update the receiver gain parameters.
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  // There is no need to update the gain if no change has occurred.
  // This way, we're nicer to the hardware.
  if (gainError != 0)
  {
    // Update the receiver gain parameters.
    setHardwareGainInDb(me.gainInDb);

    // Indicate that the gain was modified.
    me.gainWasAdjusted = 1;
  } // if
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // runHarris


/**************************************************************************

  Name: agc_displayInternalInformation

  Purpose: The purpose of this function is to display information in the
  AGC.

  Calling Sequence: agc_displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void agc_displayInternalInformation(void)
{

  (stderr,"\n--------------------------------------------\n");
  fprintf(stderr,"AGC Internal Information\n");
  fprintf(stderr,"--------------------------------------------\n");

  if (me.enabled)
  {
    fprintf(stderr,"AGC Emabled               : Yes\n");
  } // if
  else
  {
    fprintf(stderr,"AGC Emabled               : No\n");
  } // else

  fprintf(stderr,"Blanking Counter            : %u ticks\n",
          me.blankingCounter);

  fprintf(stderr,"Blanking Limit              : %u ticks\n",
          me.blankingLimit);

  fprintf(stderr,"Lowpass Filter Coefficient: %0.3f\n",
          me.alpha);

  fprintf(stderr,"Deadband                    : %u dB\n",
          me.deadbandInDb);

  fprintf(stderr,"Operating Point             : %d dBFs\n",
          me.operatingPointInDbFs);

 fprintf(stderr,"Gain                         : %u dB\n",
          me.gainInDb);

  fprintf(stderr,"RSSI (After Amp)            : %d dBFs\n",
          me.normalizedSignalLevelInDbFs);
  fprintf(stderr,"/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/\n");

  return;

} // agc_displayInternalInformation

