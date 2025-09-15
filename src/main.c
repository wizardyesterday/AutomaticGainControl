#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "AutomaticGainControl.h"

static uint32_t gainInDbFs;

void setGainCallback(uint32_t gainInDbFs)
{

  fprintf(stdout,"setGainCallback\n");

  return;

} // setGainCallback

uint32_t getGainCallback(void)
{

  fprintf(stdout,"setGainCallback\n");

  return (gainInDbFs);

} // getGainCallback
  
int main(int argc,char **argv)
{
  uint32_t i;

  gainInDbFs = 0;

  agc_init(gainInDbFs,setGainCallback,getGainCallback);

  i = 0;

  while (1)
  {
    i = i + 1;
  } // while

  return (0);

} // min
