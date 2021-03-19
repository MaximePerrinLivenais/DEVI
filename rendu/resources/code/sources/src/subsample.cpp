#include "subsample.hpp"

namespace
{

  template <class T>
  mln::image2d<T> subsample_T(const mln::image2d<T>& input)
  {
    int width = input.width();
    int height = input.height();

    int w = width / 2;
    int h = height / 2;

    mln::image2d<T> out(w, h);

    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
      {
        int x0      = 2 * x;
        int y0      = 2 * y;
        out({x,y}) = (input({x0,y0}) + input({x0+1, y0}) + input({x0, y0+1}) + input({x0, y0+1})) / 4;
      }
    return out;
  }


  template <class T>
  mln::image2d<T> upsample_T(const mln::image2d<T>& input, mln::box2d domain)
  {

    int w = domain.width();
    int h = domain.height();
    float sy = (float)input.height() / h;
    float sx = (float)input.width() / w;

    mln::image2d<T> out(w, h);
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
      {
        float x0 = x * sx;
        float y0 = y * sy;
        int lx = static_cast<int>(x0);
        int ly = static_cast<int>(y0);
        if ((lx - x0) > 0.5f)
          lx += 1;
        if ((ly - y0) > 0.5f)
          ly += 1;

        // float tmp0 = lerp(input.at(ly,lx), input.at(ly,lx+1), x0 - lx);
        // float tmp1 = lerp(input.at(ly+1,lx), input.at(ly+1,lx+1), x0 - lx);
        // float tmp2 = lerp(tmp0, tmp1, y0 - ly);
        out({x, y}) = input({lx, ly});
      }
    return out;
  }
}

mln::image2d<uint8_t> subsample(const mln::image2d<uint8_t>& input)
{
  return subsample_T(input);
}

mln::image2d<int16_t> upsample(const mln::image2d<int16_t>& input, mln::box2d domain)
{
  return upsample_T(input, domain);
}
