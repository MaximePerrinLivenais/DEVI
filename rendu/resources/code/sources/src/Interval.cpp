#include "Interval.hpp"
#include <algorithm>
#include <cassert>

bool Interval::disjoints(Interval other) const
{
  return (b < other.a) || (other.b < a);
}


bool Interval::intersects(Interval other) const
{
  return !disjoints(other);
}

float Interval::overlap(Interval x) const
{
  int u = std::max(a, x.a);
  int v = std::min(b, x.b);
  if (v <= u)
    return 0;
  else
    return float(v - u) / (x.b - x.a);
}


bool Interval::has(int v) const
{
  return a <= v && v <= b;
}

float Interval::length() const
{
  return b-a;
}


void Interval::extend(Interval other)
{
  assert(intersects(other));
  a = std::min(a, other.a);
  b = std::min(b, other.b);
}


void IntervalSet::insert(int a, int b)
{
  assert(a <= b);

  m_data.push_back({a,b});
}


bool IntervalSet::has(int v) const
{
  return std::any_of(m_data.begin(), m_data.end(), [v](auto i) { return i.has(v); });
}

bool IntervalSet::intersects(Interval i) const
{
  return std::any_of(m_data.begin(), m_data.end(), [i](auto x) { return x.intersects(i); });
}

bool IntervalSet::intersects(Interval i, float p) const
{
  return std::any_of(m_data.begin(), m_data.end(), [i,p](auto x) { return x.overlap(i) > p; });
}

