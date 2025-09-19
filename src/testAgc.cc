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

  // Assume 7 bits of magnitude for a signal.
  numberOfBits = 7;

  agc_init(operatingPointInDbFs,numberOfBits,setGainCallback,getGainCallback);

  agc_displayInternalInformation(&displayBufferPtr);
  printf("%s",displayBufferPtr);

  i = 0;

  while (1)
  {
    i = i + 1;
  } // while

  // Free resources.Ptr;
  delete[] displayBufferPtr;

  return (0);

} // min
