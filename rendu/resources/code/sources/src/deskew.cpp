#include "deskew.hpp"
#include "config.hpp"

#include <spdlog/spdlog.h>
#include <cmath>


namespace
{

  // Deskew offset detection
  // Sort *vertical* segments from left to right and display their angle (compute their average value)
  float detect_offset(const std::vector<Segment>& segments, int width)
  {
    constexpr float kBorder = 0.1f; // 10% of the total width

    fmt::memory_buffer buf;
    double sum = 0;
    int    count = 0;
    for (const auto& s : segments)
    {
      double angle = s.angle;
      if (!s.is_vertical())
        continue;

      // Dismiss segments on the left/right edges
      if (std::min(s.start.x, s.end.x) < kBorder * width ||
          std::max(s.start.x, s.end.x) > (1-kBorder) * width)
        continue;

      sum += angle;
      count++;
      fmt::format_to(buf, "{},", angle);
    }
    if (count == 0)
    {
      spdlog::info("No segment detected !");
      return 90;
    }

    spdlog::debug("Segment angles: {}", std::string_view{buf.data(), buf.size()});
    return sum / count;
  }


  PageData deskew(const PageData& pp, float angle)
  {
    PageData res = pp;
    res.image    = mln::imchvalue<uint8_t>(pp.image).set_init_value(0);

    float c      = std::cos(angle * M_PI / 180);
    int   height = pp.image.height();
    int   width  = pp.image.width();

    {
      const auto& input = pp.image;
      auto&       out   = res.image;


      const uint8_t* ilineptr = input.buffer();
      uint8_t*       olineptr = out.buffer();

      for (int y = 0; y < height; ++y)
      {
        float offset = y * c;
        for (int x = 0; x < width; ++x)
        {
          float xin = x + offset;
          int   x0  = std::floor(xin);
          int   x1  = x0 + 1;

          if (x1 <= 0 || x0 >= (width - 1))
            olineptr[x] = 255;
          else
          {
            float alpha = (xin - x0);
            olineptr[x] = (1.f - alpha) * ilineptr[x0] + alpha * ilineptr[x1];
          }
        }
        ilineptr += input.stride();
        olineptr += out.stride();
      }
    }

    for (auto& s : res.segments)
    {
      s.start.x -= s.start.y * c;
      s.end.x -= s.end.y * c;
    }
    for (auto& txt : res.texts)
    {
      txt.first.x -= txt.first.y * c;
    }

    return res;
  }
} // namespace

PageData deskew(const PageData& pp)
{
  float angle = detect_offset(pp.segments, pp.image.width());
  spdlog::info("Detected angle: {}", angle);
  return deskew(pp, angle);
}
