#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "AutomaticGainControl.h"

static uint32_t gainInDb;

static void setGainCallback(uint32_t gainInDb)
{

  fprintf(stdout,"setGainCallback() gain: %u dB\n",gainInDb);

  return;

} // setGainCallback

static uint32_t getGainCallback(void)
{

  fprintf(stdout,"getGainCallback()\n");

  return (gainInDb);

} // getGainCallback
  
int main(int argc,char **argv)
{
  uint32_t i;
  uint32_t numberOfBits;
  char *displayBufferPtr;

  // Allocate memory.
  displayBufferPtr = new char[65536];

  // Some sane value for gain.
  gainInDb = 25;

  // Assume 7 bits of magnitude for a signal.
  numberOfBits = 7;

  agc_init(gainInDb,numberOfBits,setGainCallback,getGainCallback);

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
