#pragma once
#include <chrono>


class clocker
{
public:
  clocker() { restart(); }

  int GetElapsedTimeMilliSeconds()
  {
    stop();
    auto diff = m_end - m_start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
  }

  void restart() { m_start = std::chrono::steady_clock::now(); }
  void stop() { m_end = std::chrono::steady_clock::now(); }


private:
  std::chrono::time_point<std::chrono::steady_clock> m_start;
  std::chrono::time_point<std::chrono::steady_clock> m_end;
};
