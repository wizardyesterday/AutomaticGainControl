//**************************************************************************
// file name: DbfsCalculator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block that computes decibles
// below a full scale value of a finite word length quantity.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __DBFSCALCULATOR__
#define __DBFSCALCULATOR__

#include <stdint.h>
int dbfs_init(uint32_t wordLengthInBits);
int32_t convertMagnitudeToDbFs(uint32_t signalMagnitude);

#endif // __DBFSCALCULATOR__
