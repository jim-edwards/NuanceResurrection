#ifndef CRITICALSECTION_H
#define CRITICALSECTION_H

class CriticalSection
{
public:
  CriticalSection()
  {
    InitializeCriticalSection(&criticalSection);
  }

  ~CriticalSection()
  {
    DeleteCriticalSection(&criticalSection);
  }

  void Enter()
  {
    EnterCriticalSection(&criticalSection);
  }

  void Leave()
  {
    LeaveCriticalSection(&criticalSection);
  }

private:
  CRITICAL_SECTION criticalSection;
};

#endif
