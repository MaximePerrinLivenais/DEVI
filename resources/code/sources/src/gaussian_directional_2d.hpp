#pragma once

#include <cstdint>
#include <mln/core/image/ndimage_fwd.hpp>


/// Perform a 2D gaussian filter
///
/// \param h_sigma Horizontal stddev of the filter (0 to disable horizontal filtering)
/// \param v_sigma Vertical stddev of the filter (0 to disable vertival filtering)
void gaussian2d(mln::image2d<uint8_t>& input, float h_sigma, float v_sigma, uint8_t border_value);
