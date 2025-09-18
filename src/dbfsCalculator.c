//**************************************************************************
// file name: dbfsCalculator.c
//**************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "dbfsCalculator.h"

// Allow a maximum of 31 bit wordlength.
#define MAX_WORD_LENGTH (31)

// This is the size of the magnitude to decibel lookup.
#define MAX_LOOKUP_INDEX (256)

// All private stuff is bundled in one structure.
static struct privateData
{
  // Don't run unless the system has been initialized.
  int initialized;

  // The full scale value is 2^(number of bits).
  uint32_t fullScaleValue;

  // This is used for dBFs computations.
  uint32_t fullScaleValueInDb;

  uint8_t magnitudeBuffer[16384];

  // This table is used to compute decibels for values [0,255].
  int32_t dbTable[257];
} me;

/*****************************************************************************

  Name: dbfs_init()

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of a DbfsCalculator.

  Calling Sequence: initialized = dbfs_init(wordLengthInBits)

  Inputs:

    wordLengthInBits - The number of bits in the full scale value.

 Outputs:

    initialized - A flag that indicate whether the system was properly
    initialized, and a value of zero indicates that was not initialized..

*****************************************************************************/
int dbfs_init(uint32_t wordLengthInBits)
{
  uint32_t i;
  float dbLevel;

  // Make sure this indicates we're not initialized.
  me.initialized = 0;

  if (wordLengthInBits > MAX_WORD_LENGTH)
  {
    // Clip it.
    wordLengthInBits = MAX_WORD_LENGTH;
  } // if

  // Save for later use.
  me.fullScaleValue = (1 << wordLengthInBits) - 1;

  // Note that 2's complement demands (full scale) / 2.
  me.fullScaleValueInDb = (uint32_t)(20 * log10((double)me.fullScaleValue));
  
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Construct the decibel table. The table can allow a
  // lookup that maps magnitudes from zero to 256 into
  // decibels.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 1; i <= MAX_LOOKUP_INDEX; i++)
  {
    dbLevel = 20 * log10((float)i);
    me.dbTable[i] = (int32_t)dbLevel;
  } // for 

  // Avoid minus infinity.
  me.dbTable[0] = me.dbTable[1]; 
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // This is the top level qualifier for the system to run.
  me.initialized = 1;

  return (me.initialized);

} // dbfs_init

/**************************************************************************

  Name: dbfs_convertMagnitudeToDbFs

  Purpose: The purpose of this function is to convert a signal magnitude
  to decibels referred to the full scale value.

  Calling Sequence: dbFsValue = dbfs_convertMagnitudeToDbFs(signalMagnitude)

  Inputs:

    signalMagnitude - The magnitude of the signal.

  Outputs:

    None.

**************************************************************************/
int32_t dbfs_convertMagnitudeToDbFs(
    uint32_t signalMagnitude)
{
  int32_t decibels;
  int32_t dbFsValue;

  // Initial value of no overhead.
  decibels = 0;

  if (me.initialized)
  {

  if (signalMagnitude > me.fullScaleValue)
  {
    // Clip it.
    signalMagnitude = me.fullScaleValue;
  } // if

  // Scale the signal magnitude so that it can be used as an index.
  while (signalMagnitude > MAX_LOOKUP_INDEX)
  {
    // Scale it..
    signalMagnitude /= 2;

    // Add 6dB to account for the divide by 2.
    decibels += 6;
  } // while

  // Map to decibels. 
  dbFsValue = me.dbTable[signalMagnitude];

  // Add in the compensation for any scaling.
  dbFsValue += decibels;

  // Compute to decibels below the full scale value.
  dbFsValue -= me.fullScaleValueInDb;
  } // if
  else
  {
    // Return something out of range
    dbFsValue =  -9999;
  } // else

  return (dbFsValue);

} // dbfs_convertMagnitudeToDbFs
