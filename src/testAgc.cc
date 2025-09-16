#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "AutomaticGainControl.h"

//static uint32_t gainInDbFs;
static uint32_t gainInDb;
static char displayBuffer[65536];

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
  char *displayBufferPtr;

  // Reference the base address of the display buffer.
  displayBufferPtr = displayBuffer;

  // Some sane value for gain.
  gainInDb = 25;

  agc_init(gainInDb,setGainCallback,getGainCallback);

  agc_displayInternalInformation(&displayBufferPtr);
  printf("%s",displayBuffer);

  i = 0;

  while (1)
  {
    i = i + 1;
  } // while

  return (0);

} // min
