#include <CoreTypes.hpp>
#include <algorithm>
#include "config.hpp"


void Box::merge(Box o)
{
  int x0 = std::min(x, o.x);
  int y0 = std::min(y, o.y);
  int x1 = std::max(this->x1(), o.x1());
  int y1 = std::max(this->y1(), o.y1());

  x      = x0;
  y      = y0;
  width  = x1 - x0;
  height = y1 - y0;
}

void Box::inflate(int k)
{
  width  = std::max(0, width + 2 * k);
  height = std::max(0, height + 2 * k);
  x -= k;
  y -= k;
}

bool Box::has(Point2D p) const
{
  return x <= p.x && p.x < (x + width) && y <= p.y && p.y < (y + height);
}

bool Box::has(Segment s) const
{
  return has(s.start) && has(s.end);
}




namespace
{
  bool disjoints(int a1, int a2, int b1, int b2)
  {
    return (a2 <= b1) || (b2 <= a1);
  }

  // Check if segment [a1, a2) intersects [b1, b2)
  bool intersects(int a1, int a2, int b1, int b2)
  {
    return !disjoints(a1, a2, b1, b2);
  }
}

bool Box::intersects(Box o) const
{
  return ::intersects(this->x, this->x1(), o.x, o.x1()) || //
         ::intersects(this->y, this->y1(), o.y, o.y1());
}

bool Box::empty() const
{
  return (width <= 0) || (height <= 0);
}



bool Segment::is_horizontal() const
{
  auto a = angle <= 90 ? angle : (180 - angle);
  return a < kAngleTolerance;
}

bool Segment::is_vertical() const
{
  return std::abs(angle - 90) < kAngleTolerance;
}

void Segment::scale(float s)
{
  start.x = static_cast<int>(start.x * s);
  start.y = static_cast<int>(start.y * s);
  end.x   = static_cast<int>(end.x * s);
  end.y   = static_cast<int>(end.y * s);
  length  = length * s;
}

