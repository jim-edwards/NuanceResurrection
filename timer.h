#ifndef timerH
#define timerH

#include <cstdint>

class MPE;

struct _currenttime {
  int32_t sec;
  int32_t min;
  int32_t hour;
  int32_t wday;
  int32_t mday;
  int32_t month;
  int32_t year;
  int32_t isdst;       /* flag: for definitions, see below */
  int32_t timezone;    /* minutes west of Greenwich */
  int32_t reserved[3]; /* reserved for future expansion */
};

void InitializeTimingMethod();
void DeInitTimingMethod();

uint64 useconds_since_start();

void TimeElapsed(MPE &mpe);
void TimeOfDay(MPE &mpe);
void TimerInit(const uint32 whichTimer, const uint32 rate);
void TimerInit(MPE &mpe);

#endif
