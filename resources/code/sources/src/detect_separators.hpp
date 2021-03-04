#pragma once


#include <CoreTypes.hpp>
#include <mln/core/image/ndimage_fwd.hpp>
#include <vector>
#include <cstdint>


std::vector<Segment> detect_separators(const mln::image2d<uint8_t>& input);
