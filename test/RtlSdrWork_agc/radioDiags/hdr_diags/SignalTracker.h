//**************************************************************************
// file name: SignalTracker.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal-operated squelch function.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __SIGNALTRACKER__
#define __SIGNALTRACKER__

#include <stdint.h>

#include "SignalDetector.h"

#define SIGNALTRACKER_NOISE (0)
#define SIGNALTRACKER_STARTOFSIGNAL (1)
#define SIGNALTRACKER_SIGNALPRESENT (2)
#define SIGNALTRACKER_ENDOFSIGNAL (3)

class SignalTracker
{
  //***************************** operations **************************

  public:

  enum TrackerState {NoSignal, Tracking};

  SignalTracker(void);
  ~SignalTracker(void);

  void reset(void);
  uint16_t run(bool signalIsPresent);

  //***************************** attributes **************************
  private:

  TrackerState state;
};

#endif // __SIGNALTRACKER__
