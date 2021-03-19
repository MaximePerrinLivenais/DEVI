#pragma once

#include <CoreTypes.hpp>
#include <mln/core/image/ndimage.hpp>
#include <string>

struct PageData
{
  mln::image2d<uint8_t>      image;
  std::vector<std::pair<Box, std::string>> texts;
  std::vector<Segment>                     segments;
};

struct ApplicationData
{
  PageData                            original; // Original image
  PageData                            deskewed; // Original image deskewed

  // In (scale 1)
  std::vector<Segment>  segments;
  mln::image2d<uint8_t> input;
  mln::image2d<uint8_t> blocks;
  mln::image2d<uint8_t> blocks2;
  mln::image2d<int16_t> ws;
};
