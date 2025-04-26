#include <chrono>

std::chrono::time_point<std::chrono::high_resolution_clock> counterStart, counterEnd, dummyCounter;

double GetTimeDeltaMs()
{
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(counterEnd - counterStart);

  double milliseconds = (double)microseconds.count() / 1000;

  return milliseconds;
}

void StartPerformanceTimer()
{
  counterStart = std::chrono::high_resolution_clock::now();
}

void StopPerformanceTimer()
{
  counterEnd = std::chrono::high_resolution_clock::now();
}

double GetPerformanceTimerOverheadMs()
{
  StartPerformanceTimer();
  dummyCounter = std::chrono::high_resolution_clock::now();
  StopPerformanceTimer();

  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(counterEnd - counterStart);

  double milliseconds = (double)microseconds.count() / 1000;

  return milliseconds;
}
