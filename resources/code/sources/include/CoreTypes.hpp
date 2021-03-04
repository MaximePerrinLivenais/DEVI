#pragma once

struct Point2D
{
  int x, y;
};

struct Segment
{
  Point2D start, end; // Segment end-points (with start.y <= end.y)
  double width;      // Segment width
  double nfa;        // Segment confidence
  double length;     // Segment length
  double angle;      // Segment angle (in degree between 0 - 180)

  bool is_horizontal() const;
  bool is_vertical() const;

  void scale(float s);
};


struct Box
{
  int x;
  int y;
  int width;
  int height;

  int x0() const { return x; }
  int y0() const { return y; }
  int x1() const { return x + width; }
  int y1() const { return y + height; }


  void merge(Box other);
  void inflate(int b);
  bool intersects(Box other) const;
  bool empty() const;
  bool has(Point2D p) const;
  bool has(Segment s) const;
};




enum e_force_indent
{
  FORCE_NONE  = 0,
  FORCE_LEFT  = 1,
  FORCE_RIGHT = 2,
};
