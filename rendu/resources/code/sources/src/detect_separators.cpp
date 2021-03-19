#include "detect_separators.hpp"

#include <mln/core/algorithm/transform.hpp>
#include <mln/core/image/ndimage.hpp>

extern "C"
{
#include <lsd.h>
}

#include <spdlog/spdlog.h>

#include <cmath>

// Configuration
static int kValidRegionBorder = 100;
static int kMinLength = 100;




std::vector<Segment> detect_separators(const mln::image2d<uint8_t>& input)
{
  // Convert to double for LSD with no-border
  mln::image_build_params params;
  params.border = 0;

  mln::image2d<double> f(input.domain(), params);
  mln::transform(input, f, [](uint8_t x) { return static_cast<double>(x); });


  // Run LSD
  int n_segment = 0;
  double* results;
  {
    int*    tmp;
    int     x, y;
    double* buf         = f.buffer();
    double  scale       = 0.5;
    double  sigma_scale = 0.6; /* Sigma for Gaussian filter is computed as
                               sigma = sigma_scale/scale.                    */
    double quant = 2.0;        /* Bound to the quantization error on the
                                   gradient norm.                                */
    double ang_th     = 22.5;  /* Gradient angle tolerance in degrees.           */
    double log_eps    = 2.0;   /* Detection threshold: -log10(NFA) > log_eps     */
    double density_th = 0.7;   /* Minimal density of region points in rectangle. */
    int    n_bins     = 1024;  /* Number of bins in pseudo-ordering of gradient
                                  modulus.                                       */

    results = LineSegmentDetection(&n_segment, buf, f.width(), f.height(), scale, sigma_scale, quant, ang_th, log_eps, density_th,
                                   n_bins, &tmp, &x, &y);
    //auto tmpi = mln::image2d<int>::from_buffer((void*)tmp, mln::box2d{{0, 0}, {y, x}});
    //mln::io::imsave(tmpi, "tmp.tiff");
  }

  // Convert back the segments
  std::vector<Segment> segments;
  double* res = results;
  for (int i = 0; i < n_segment; ++i)
  {
    Segment s;
    s.start = {(int)res[0], (int)res[1]};
    s.end   = {(int)res[2], (short)res[3]};
    if (s.end.y < s.start.y)
      std::swap(s.start, s.end);

    s.length = std::hypot(res[2] - res[0], res[3] - res[1]);
    s.angle  = std::atan2(res[3] - res[1], res[2] - res[0]) * 180 / M_PI;
    if (s.angle < 0)
      s.angle += 180.;
    s.width = res[4];
    s.nfa   = res[6];


    segments.push_back(s);
    res += 7;
  }
  free(results);


  // Filter segments
  auto end          = segments.end();
  auto valid_region = f.domain();
  valid_region.inflate(-kValidRegionBorder);
  end = std::remove_if(segments.begin(), end, [valid_region](const auto& s) {
    bool valid = true;
    valid &= valid_region.has(mln::point2d{s.start.x, s.start.y});
    valid &= valid_region.has(mln::point2d{s.end.x, s.end.y});
    valid &= s.length >= kMinLength;
    return !valid;
  });

  segments.resize(end - segments.begin());

  // Debug
  spdlog::debug("LSD - Detected segments");
  for (const auto& s : segments)
  {
    spdlog::debug("x1={} y1={} x2={} y2={} width={} nfa={} length={} angle={}", s.start.x, s.start.y, s.end.x,
                  s.end.y, s.width, s.nfa, s.length, s.angle);
  }

  return segments;
}

/*
std::vector<mln::box2d> detect_layout(mln::box2d domain, std::vector<segment2d> segments)
{
  constexpr float kMergeAngleTolerance = 3;
  constexpr float kMaximalHorizontalMerge = 15;
  constexpr float kMaximamVerticalGap = 10000;



  // Remove non-vertical segments
  auto end = std::remove_if(segments.begin(), segments.end(), [](const auto& s) { return std::abs(s.angle - 90) > kVerticalAngleTolerance; });


  // Sort segment by their the (x,y) coords of their start points (scan order)
  int n = end - segments.begin();
  std::sort(segment.begin(), segment.begin() + n);


  // 1. Merge cloase vertical segments.
  {
    for (int i = 0; i < n;)
    {
      auto& s1 = segments[i];
      for (int j = i + 1; j < n;)
      {
        auto& s2 = segments[j];
        // Test if the segment is duplicate
        bool pred = true;
        pred &= std::abs(s1.angle - s2.angle) < kMergeAngleTolerance;
        pred &= std::abs(s1.start[1] - s2.start[1]) < kMaximalHorizontalMerge;
        pred &= std::abs(s1.end[1] - s2.end[1]) < kMaximalHorizontalMerge;
        if (pred)
        {
          s1.start[0] = std::min(s1.start[0], s2.start[0]);
          s1.end[0] = std::max(s1.end[0], s2.max[0]);
        }
      }

      i++;
    }
  }



}
*/
