#pragma once

#include <mln/core/image/ndimage.hpp>
#include <vector>




/// Given an array A[i] of integer
/// A boolean array B[i] >= threshold is created  and the function
/// returns the 0-section that are separated by the the 1-section (i.e. returns the minima of B)
/// * The \p min_separator_size is used to has the size opening.
/// * The special value INT_MAX (or UINT8_MAX in the second version) is used to force the split
/// * The leading and the trailing 1-sections are ignored in the process
///
/// split_section([1 0 0 0 1 1 0 0 1 1 1 0 0 1]) with threshold = 1; min_sep_size = 3
///                  <----------->       <->
/// returns [ [1,8), [11, 13)]
std::vector<std::pair<int, int>>
split_section(const int* data, std::size_t n, int threshold, int min_seperator_size = 0);

std::vector<std::pair<int, int>>
split_section(const uint8_t* data, std::size_t n, int min_seperator_size = 0);



/// Given an array A[i] of integer and a threshold t.
/// Returns argmin(A[i] < t)
int detect_leading_space(const int* values, std::size_t n, int threshold);

/// Given a boolean array A[i], return the posiition of the first 0
/// Returns argmin(A[i] == 0)
int detect_leading_space(const uint8_t* values, std::size_t n);




/// Sum-up the values of an image along the given axes
std::vector<int> sum_along_x_axis(const mln::image2d<uint8_t>& input);
std::vector<int> sum_along_y_axis(const mln::image2d<uint8_t>& input);

enum class Axis
{
  Y = 0,
  X = 1
};

/// 
//void count_less_than(const mln::experimental::image2d<uint8_t>& input, uint8_t threshold, float* out, Axis axis);
//std::vector<float> count_less_than(const mln::experimental::image2d<uint8_t>& input, uint8_t threshold, Axis axis);

// Return true column-wise or row-wise if the pourcent of black pixel (below white_level) is less than (strict) a given
// a threshold in percentage.
void is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, float percentile, Axis axis,
                                         uint8_t* out);


std::vector<uint8_t> is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, float percentile,
                                                         Axis axis);

/*
/// Compute the rank value of an image along the given axes
/// \param rank Rank value in the range [0,1)
void rank_along_x_axis(const mln::experimental::image2d<uint8_t>& input, float rank, int* out);
void rank_along_y_axis(const mln::experimental::image2d<uint8_t>& input, float rank, int* out);
std::vector<int> rank_along_x_axis(const mln::experimental::image2d<uint8_t>& input, float rank);
std::vector<int> rank_along_y_axis(const mln::experimental::image2d<uint8_t>& input, float rank);
*/

int rank(const mln::image2d<uint8_t>& input, float r);

