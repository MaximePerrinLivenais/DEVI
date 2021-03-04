#pragma once
#include <vector>

// Closed interval of integers
struct Interval
{
  int a;
  int b;


  float length() const;
  bool has(int v) const;
  bool intersects(Interval other) const;
  bool disjoints(Interval other) const;
  void extend(Interval other);

  // Return the pourcentage of X overlaps with the segment
  // Computes AREA(X ∩ SELF) / AREA(X)
  float overlap(Interval X) const;
};



class IntervalSet
{
public:
  IntervalSet() = default;

  // Insert the values defined by the interval [a,b]
  void insert(int a, int b);

  // True if v belongs to one of the interval
  bool has(int v) const;

  // True if interval intersects one of the interval
  bool intersects(Interval i) const;

  // True if x overlaps with another segment with a given pourcentage
  // Computes AREA(X ∩ S) / AREA(X) > p
  //
  //  \param p is in [0,1]
  bool intersects(Interval x, float p) const;

private:
  std::vector<Interval> m_data;
};
