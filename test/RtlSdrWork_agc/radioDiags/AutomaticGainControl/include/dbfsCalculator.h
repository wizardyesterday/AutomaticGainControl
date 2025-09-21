//**************************************************************************
// file name: dbfsCalculator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block that computes decibles
// below a full scale value of a finite word length quantity.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __DBFSCALCULATOR__
#define __DBFSCALCULATOR__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int dbfs_init(uint32_t wordLengthInBits);
int32_t dbfs_convertMagnitudeToDbFs(uint32_t signalMagnitude);

#ifdef __cplusplus
}
#endif

#endif // __DBFSCALCULATOR__
