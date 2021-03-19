#include "DOMBuilder_helpers.hpp"

#include <mln/core/range/foreach.hpp>
#include <mln/core/algorithm/for_each.hpp>
#include <numeric>


std::vector<int> sum_along_x_axis(const mln::image2d<uint8_t>& input)
{
  int            height  = input.height();
  int            width   = input.width();
  auto           sum     = std::vector<int>(height);
  const uint8_t* lineptr = input.buffer();

  for (int y = 0; y < height; ++y)
  {
    sum[y] = std::accumulate(lineptr, lineptr + width, 0);
    lineptr += input.stride();
  }
  return sum;
}

std::vector<int> sum_along_y_axis(const mln::image2d<uint8_t>& input)
{
  int width = input.width();
  int x0    = input.domain().x();

  std::vector<int> sum(width);
  mln_foreach (auto&& px, input.pixels())
  {
    auto x = px.point().x() - x0;
    sum[x] += px.val();
  }
  return sum;
}


std::vector<std::pair<int, int>> split_section(const int* data, std::size_t n_, int threshold, int min_seperator_size)
{
  std::vector<std::pair<int, int>> sections;

  int i = 0;
  int n = static_cast<int>(n_);

  // Ignore the leading 1-sections
  while (i < n && data[i] >= threshold)
    i++;

  int left = i;
  while (i < n)
  {
    // Advance to the end of the 0-section
    while (i < n && data[i] < threshold)
      i++;

    int right = i;
    // Advance to the en of the 1-section
    bool forced = false;
    while (i < n && data[i] >= threshold) {
      forced |= (data[i] == INT_MAX);
      i++;
    }

    int sep_size = i - right;
    if (i == n || sep_size >= min_seperator_size || forced)
    {
      sections.push_back(std::make_pair(left, right));
      left = i;
    }
  }
  return sections;
}

std::vector<std::pair<int, int>> split_section(const uint8_t* data, std::size_t n, int min_seperator_size)
{
  std::vector<std::pair<int, int>> sections;

  int i = 0;

  // Ignore the leading 1-sections
  while (i < static_cast<int>(n) && data[i])
    i++;

  int left = i;
  while (i < static_cast<int>(n))
  {
    // Advance to the end of the 0-section
    while (i < static_cast<int>(n) && !data[i])
      i++;

    int right = i;
    bool forced = false;
    // Advance to the en of the 1-section
    while (i < static_cast<int>(n) && data[i]) {
      forced |= (data[i] == UINT8_MAX);
      i++;
    }

    int sep_size = i - right;
    if (i == static_cast<int>(n) || sep_size >= min_seperator_size || forced)
    {
      sections.push_back(std::make_pair(left, right));
      left = i;
    }
  }
  return sections;
}



int detect_leading_space(const int* values, std::size_t n, int threshold)
{
  std::size_t i = 0;
  for (; i < n; i++)
    if (values[i] < threshold)
      break;
  return i;
}

int detect_leading_space(const uint8_t* values, std::size_t n)
{
  std::size_t i = 0;
  for (; i < n; i++)
    if (values[i])
      break;
  return i;
}



/*
void count_less_than(mln::experimental::image2d<uint8_t> input, uint8_t threshold, float* out, int axis)
{
  std::vector<int> count;
  int n;
  if (axis == Axis::Y)
  {
    n = input.height();
    count.resize(input.width(), 0);
    mln_foreach_new(auto px, input.new_pixels())
      count[px.point().x()] += (px.val() < threshold);
  }
  else
  {
    n = input.width();
    count.resize(input.height(), 0);
    mln_foreach_new(auto px, input.new_pixels())
      count[px.point().y()] += (px.val() < threshold);
  }
  std::transform(std::begin(count), std::end(count), out, [n](int c) { return c / static_cast<float>(n); });
}

std::vector<float> count_less_than(const mln::experimental::image2d<uint8_t>& input, uint8_t threshold, int axis)
{
  std::vector<float> out;
  out.resize((axis == Axis::Y) ? input.width : input.height);
  count_less_than(input, threshold, out.data(), axis);
  return out;
}
*/

void is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, float percentile,
                                         Axis axis, uint8_t* out)
{
  std::vector<int> count;
  int              n;

  if (axis == Axis::Y)
  {
    int x0 = input.domain().x();
    n = input.height();
    count.resize(input.width(), 0);
    mln_foreach (auto px, input.pixels())
      count[px.point().x() - x0] += (px.val() < white_level);
  }
  else
  {
    int y0 = input.domain().y();
    n = input.width();
    count.resize(input.height(), 0);
    mln_foreach (auto px, input.pixels())
      count[px.point().y() - y0] += (px.val() < white_level);
  }

  float np = percentile * n;
  std::transform(std::begin(count), std::end(count), out, [np](int c) { return c < np; });
}

std::vector<uint8_t> is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level,
                                                         float percentile, Axis axis)
{
  std::vector<uint8_t> out;
  out.resize((axis == Axis::Y) ? input.width() : input.height());
  is_number_of_black_pixels_less_than(input, white_level, percentile, axis, out.data());
  return out;
}




namespace
{

  std::size_t get_rank(unsigned* histogram, std::size_t n, float rank)
  {
    std::partial_sum(histogram, histogram + n, histogram);
    unsigned r = static_cast<unsigned>(rank * histogram[n - 1]);
    std::size_t idx = std::lower_bound(histogram, histogram + n, r) - histogram;
    return idx;
  }
}

/*
void rank_along_x_axis(const mln::experimental::image2d<uint8_t>& input, float rank, int* rnk)
{
  std::array<unsigned, 256> histogram;

  int  height = input.nrows();
  int  width  = input.ncols();
  int  y0     = input.domain().pmin[0];
  int  x0     = input.domain().pmin[1];

  for (int y = 0; y < height; ++y)
  {
    auto* lineptr = &input.at(y0 + y, x0);
    histogram.fill(0);
    std::for_each(lineptr, lineptr + width, [&](uint8_t x) { histogram[x]++; });
    rnk[y] = get_rank(histogram.data(), histogram.size(), rank);
  }
}

std::vector<int> rank_along_x_axis(const mln::experimental::image2d<uint8_t>& input, float rank)
{
  int  height = input.nrows();
  auto rnk    = std::vector<int>(height);

  rank_along_x_axis(input, rank, rnk.data());
  return rnk;
}



void rank_along_y_axis(const mln::experimental::image2d<uint8_t>& input, float rank, int* res)
{
  using H = std::array<unsigned, 256>;
  int width = input.ncols();
  int x0    = input.domain().pmin[1];

  std::vector<H> histograms(width);
  std::for_each(histograms.begin(), histograms.end(), [](auto& h) { h.fill(0); });


  mln_foreach_new (auto&& px, input.new_pixels())
  {
    auto x = px.point()[1] - x0;
    histograms[x][px.val()]++;
  }

  std::transform(histograms.begin(), histograms.end(), res,
                 [rank](auto& h) { return get_rank(h.data(), h.size(), rank); });
}

std::vector<int> rank_along_y_axis(const mln::image2d<uint8_t>& input, float rank)
{
  int              width = input.ncols();
  std::vector<int> res(width);
  rank_along_y_axis(input, rank, res.data());
  return res;
}
*/

int rank(const mln::image2d<uint8_t>& input, float r)
{
  std::array<unsigned, 256> histogram = {0};
  mln::for_each(input, [&histogram](int v) { histogram[v]++; });
  int rnk = get_rank(histogram.data(), histogram.size(), r);
  return rnk;
}
