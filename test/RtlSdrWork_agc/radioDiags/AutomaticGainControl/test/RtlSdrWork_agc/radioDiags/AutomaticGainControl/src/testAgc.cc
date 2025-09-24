//*******************************************************************
// File: testAgc.c
// This program provides example code for initialization and
// operation of the automatic gain control.  Feel free to modify
// the program to suit your needs.
//*******************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "AutomaticGainControl.h"

static uint32_t gainInDb;


/**************************************************************************

  Name: setGainCallback

  Purpose: The purpose of this function is to perform the actions of
  what a real application would perform.  The real callback would
  convert the gain into linear units (if necessary), and set the
  amplifier gain.  If the amplifer gain is already in decibels, no
  conversion is necessary.
 

  NOTE: This callback provides a template for the developer.

  Calling Sequence: setGainCallback(gainInDb)

  Inputs:

    gainInDb The gain, in decibels of the amplifier for which the AGC
    is to control..

  Outputs:

    None.

**************************************************************************/
static void setGainCallback(uint32_t gainInDb)
{

  fprintf(stdout,"setGainCallback() gain: %u dB\n",gainInDb);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Hardware-specific computations are performed here.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Perform any conversions thare necessary to set the
  // amplifier gain.
  // Set the amplifier gain.
 //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // setGainCallback

/**************************************************************************

  Name: getGainCallback

  Purpose: The purpose of this function is to perform the actions of
  what a real application would perform.  The real callback would
  retrieve the amplifier gain, make sure the gain is converted to
  decibels (if needed), and retuen the gain, in decibels, to the caller.

  NOTE: This callback provides a template for the developer.

  Calling Sequence: gainInDb = getGainCallback()

  Inputs:

    None.

  Outputs:

    gainInDb The gain, in decibels of the amplifier for which the AGC
    is to control.

**************************************************************************/
static uint32_t getGainCallback(void)
{

  fprintf(stdout,"getGainCallback()\n");

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Hardware-specific computations are performed here.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Retrieve the amplifier gain.
  // Convert to dBFS.
  // gainInDb = converted gain.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return (gainInDb);

} // getGainCallback

//************************************************************
// Mainline code.
//************************************************************  
int main(int argc,char **argv)
{
  uint32_t i;
  uint32_t numberOfBits;
  int32_t operatingPointInDbFs;
  char *displayBufferPtr;

  // Allocate memory.
  displayBufferPtr = new char[65536];

  // Some sane value for gain.
  operatingPointInDbFs = -12;

  // 8-bit sample size with 1 sign bit and 7 magnitude bits, as is the
  // case for an 8-bit 2's complement value.
  numberOfBits = 7;

  // Initalize the AGC with an operating point of -8dBFs.
  agc_init(-8,numberOfBits,setGainCallback,getGainCallback);

  // Set the convergence contstant.
  agc_setAgcFilterCoefficient(0.7);

  // Set deadband of +-1dB.
  agc_setDeadband(1);

  // Set blank timeout of 2 invocations.  This will mean that the AGC
  // will be run on every invocation.
  agc_setBlankingLimit(1);

  // Enable the AGC.  his would be done elsewhere by the user.
  agc_enable();

  // Let's display the AGC parameters.
  agc_displayInternalInformation(&displayBufferPtr);
  printf("%s\n\n",displayBufferPtr);

  // Enable the receiver and wait 3 seconds (contrived) to stabilize.
  // Don't use the sleep() function in an actual application.
  sleep(2);

  // The receiver is now stable, so let's send it a signal magnitufe.
  agc_acceptData(80);

  // Display the operational AGC parameters.
  // NOte that on a live system, you will sometimes see a nonzero
  // Blanking timer calue, depending upon the signal environment.
  agc_displayInternalInformation(&displayBufferPtr);
  printf("%s\n\n",displayBufferPtr);

  i = 0;

  while (1)
  {
    i = i + 1;
  } // while

  // Free resources.Ptr;
  delete[] displayBufferPtr;

  return (0);

} // min
