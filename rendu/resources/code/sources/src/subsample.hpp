#pragma once

#include <mln/core/image/ndimage.hpp>

mln::image2d<uint8_t> subsample(const mln::image2d<uint8_t>& input);
mln::image2d<int16_t> upsample(const mln::image2d<int16_t>& input, mln::box2d domain);
