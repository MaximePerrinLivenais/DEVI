#include "DOMBlocksExtractor.hpp"

#include <spdlog/spdlog.h>

#include "config.hpp"
#include "DOMBuilder_helpers.hpp"
#include "Interval.hpp"

#include <mln/core/se/periodic_line2d.hpp>
#include <mln/morpho/opening.hpp>
#include <mln/morpho/rank_filter.hpp>
#include <mln/io/imsave.hpp>

#include <climits>


namespace
{
  constexpr int kWhiteThreshold = 200;   // Gray value for which a pixel is considered white (background)


  // Detect the left border
  std::pair<int, int> detect_border(const int* partial_sum, std::size_t n, int length)
  {
    constexpr int kBorderPageRatio = 10; // 1/kBorderPageRatio Maximal crop size
    constexpr float kFullLineWhite  = 0.95f; // Number of white pixels for which we consider a line/column to be white
    int l = n / kBorderPageRatio;
    int r = (kBorderPageRatio - 1) * n / kBorderPageRatio;
    int sum_threshold = kWhiteThreshold * length * kFullLineWhite;
    if (partial_sum[l]  < sum_threshold) {
      while (l > 0 && partial_sum[l] < sum_threshold)
        l--;
    } else {
      while (l < r && partial_sum[l] > sum_threshold)
        l++;
    }

    if (partial_sum[r]  < sum_threshold) {
      while (r < int(n-1) && partial_sum[r] < sum_threshold)
        r++;
    } else {
      while (r > l && partial_sum[r] > sum_threshold)
        r--;
    }
    return std::make_pair(l, r+1);
  }



  struct DOMBlocksExtractor : public DOMElementVisitor
  {

    static constexpr int kExtraMargin = 2; // Add this as a margin to block

    DOMBlocksExtractor() = default;

    std::vector<Segment>  segments;
    mln::image2d<uint8_t> blocks1; // Image prerpocessed for block processing
    mln::image2d<uint8_t> blocks2; // Image prerpocessed for block processing (with vertical lines removed)


    // Split vertically - (horizontal separation)
    void vsplit(DOMElement* hsec, int level)
    {
      using mln::point2d;
      using mln::box2d;


      box2d region(hsec->bbox.x, hsec->bbox.y, hsec->bbox.width, hsec->bbox.height);

      // Retrieve segments in the region
      std::vector<Segment> region_segments;
      std::vector<Segment> hor_segments;
      IntervalSet          ver_segments;

      {
        auto outer_region = hsec->bbox;
        auto inner_region = hsec->bbox;
        inner_region.inflate(-10);

        for (const Segment& seg : segments)
        {
          if (outer_region.has(seg))
            region_segments.push_back(seg);
          if (seg.is_horizontal() && outer_region.has(seg))
            hor_segments.push_back(seg);
          else if (seg.is_vertical() && inner_region.has(seg))
            ver_segments.insert(seg.start.y, seg.end.y);
        }
      }

      // At level 0, we consider a blank line as 90% of blank pixels (because of separators)
      // At level 1, we consider a blank line if:
      ///   1. There are 95% of blank pixels on the 1st 1/3rd (because a line can be composed of a single word as a large as a centered separator )
      ///   2. There are 70% of blank pixels on the 2nd 1/3rd (because a title might be centered and we dont want separator to considered as text).
      std::vector<uint8_t> ysum;
      if (hsec->type() == DOMCategory::PAGE)
      {
        //ysum = rank_along_x_axis(blocks1.clip(region), 0.05f);
        ysum = is_number_of_black_pixels_less_than(blocks1.clip(region), kWhiteThreshold, 0.10f, Axis::X);
      }
      else
      {
        int h = region.height();
        int w = region.width();
        ysum.resize(2 * h);

        box2d roi = region;
        roi.set_width(w / 3);
        is_number_of_black_pixels_less_than(blocks1.clip(roi), kWhiteThreshold, 0.05f, Axis::X, ysum.data()); // 95% on 1st third
        // rank_along_x_axis(blocks1.clip(roi), 0.05f, ysum.data());

        roi.tl().x() = region.x() + w / 3;
        roi.br().x() = region.x() + 2 * (w / 3);
        is_number_of_black_pixels_less_than(blocks1.clip(roi), kWhiteThreshold, 0.3f, Axis::X, ysum.data() + h); // 70% on the 2nd third
        // rank_along_x_axis(blocks1.clip(roi), 0.3f, ysum.data() + h); // 50% on 2nd third

        for (int i = 0; i < h; ++i)
          ysum[i] = std::min(ysum[i], ysum[i + h]); // Min <=> Logical And
        ysum.resize(h);
      }

      // For every horizontal segment > 25% of the block width, force the split
      {
        for (const auto& s : hor_segments)
          {
            if (s.length < 0.25f * region.width())
              continue;

            spdlog::debug("Horizontal split forced by segment (y={}, x1={} x2={} angle={})", s.start.y, s.start.x,
                          s.end.x, s.angle);
            int y0 = std::max(0, s.start.y - region.y() - 3);
            int y1 = std::min(s.end.y + 3, region.br().y()) - region.y();
            for (int y = y0; y < y1; ++y)
              ysum[y] = 255; // White out
          }
      }


      auto DOM = split_section(ysum.data(), ysum.size());
      spdlog::debug("{:<{}}** Horizontal split - number of regions={}", "", level * 2, DOM.size());

      std::unique_ptr<DOMElement>* last = nullptr;
      int lasty;
      for (auto [y0, y1] : DOM)
      {
        int h = std::abs(y1 - y0);

        // If the height is small, we probably have detected a separator => skip
        if (h < 0.25f * (kLineHeight / 2.f))
        {
          continue;
        }

        // Try to extend the previous block, or create a new one
        // 1. Test if the two blocks are too close
        if (last && (y0 - lasty) < 0.5f * kLayoutBlockOpeningHeight)
        {
          spdlog::debug("{:<{}} The current block [{}-{}] merge with the previous one lasty={} (too small {} < {})", "",
                        level * 2, y0, y1, lasty, (y0 - lasty), 0.5f * kLayoutBlockOpeningHeight);
          lasty = y1;
          y0 = (*last)->bbox.y;
          y1 += hsec->bbox.y;
        }
        // 2. Test if the block split intersects a vertical segment at 80%
        else if (last && ver_segments.intersects({lasty + hsec->bbox.y, y0 + hsec->bbox.y}, 0.80f))
        {
          spdlog::debug("{:<{}} The current block [{}-{}] merge with the previous one lasty={} because of a segment", "", level * 2, y0, y1, lasty);
          lasty = y1;
          y0 = (*last)->bbox.y;
          y1 += hsec->bbox.y;
        }
        else
        {
          last  = &hsec->children.emplace_back();
          lasty = y1;
          y0 += hsec->bbox.y - kExtraMargin;
          y1 += hsec->bbox.y + kExtraMargin;
        }

        // Determine the section type
        if ((y1 - y0) < (3 * (kLineHeight / 2.f))) // We have a title box (with height < 3*lineHeight)
        {
          if (hsec->type() == DOMCategory::PAGE)
            last->reset(new DOM::title_level_1());
          else if (hsec->type() == DOMCategory::COLUMN_LEVEL_1)
            last->reset(new DOM::title_level_2());
          else
            throw std::runtime_error("Invalid document layout.");
        }
        else
        {
          if (hsec->type() == DOMCategory::PAGE)
            last->reset(new DOM::section_level_1());
          else if (hsec->type() == DOMCategory::COLUMN_LEVEL_1)
            last->reset(new DOM::section_level_2());
          else
            throw std::runtime_error("Invalid document layout.");
        }
        y0            = std::max(region.y(), y0);
        y1            = std::min(region.br().y(), y1);
        (*last)->bbox = {region.x(), y0, region.width(), (y1 - y0)};
        spdlog::debug("{:<{}} Detected y-section [{}--{}]", "", level * 2, y0, y1);
      }


      int new_level = level + 1;
      for (auto& sec : hsec->children)
      {
        spdlog::debug("{:<{}} Processing y-section [y={},h={}]", "", level * 2, sec->bbox.y, sec->bbox.height);
        DOMBlocksExtractor parser;
        parser.segments = region_segments;
        parser.blocks1 = this->blocks1;
        parser.blocks2 = this->blocks2;
        sec->accept(parser, static_cast<void*>(&new_level));
      }
    }

    // Split horizontally (vertical separation)
    void hsplit(DOMElement* vsec, int level)
    {
      using mln::box2d;
      using mln::point2d;

      const box2d region(vsec->bbox.x, vsec->bbox.y, vsec->bbox.width, vsec->bbox.height);


      // Retrieve segments in the region
      std::vector<Segment> region_segments;
      std::vector<Segment> ver_segments;

      for (const Segment& seg : segments)
        if (region.has(point2d{seg.start.x, seg.start.y}) && region.has(point2d{seg.end.x, seg.end.y}))
        {
          region_segments.push_back(seg);
          if (seg.is_vertical())
            ver_segments.push_back(seg);
        }


      auto rnks = is_number_of_black_pixels_less_than(blocks2.clip(region), kLayoutWhiteLevel, 0.02f, Axis::Y);
      // std::vector<int> rnks    = rank_along_y_axis(blocks2.clip(region), 0.02f);



      // For every vertical segment, force the split
      {
        for (const auto& s : ver_segments)
          {
            spdlog::debug("Vertical split forced by segment (x={}, y1={} y2={} angle={})", s.start.x, s.start.y, s.end.y, s.angle);
            float x  = (s.start.x + s.end.x) / 2.f - region.x();
            int   x0 = std::max(0.f, x - 3 * kExtraMargin);
            int   x1 = std::min((float)region.width(), x + 3 * kExtraMargin);
            for (int x = x0; x < x1; ++x)
              rnks[x] = UINT8_MAX; // White out
          }
      }

      auto columns = split_section(rnks.data(), rnks.size(), 0.75f * (kColumnSpacing / 2.f));

      spdlog::debug("{:<{}}** Vertical split - number of regions={}", "", level * 2, columns.size());
      for (auto [x0, x1] : columns)
      {
        std::unique_ptr<DOMElement> sec;
        if (vsec->type() == DOMCategory::SECTION_LEVEL_1)
          sec = std::make_unique<DOM::column_level_1>();
        else if (vsec->type() == DOMCategory::SECTION_LEVEL_2)
          sec = std::make_unique<DOM::column_level_2>();
        else
          throw std::runtime_error("Invalid document layout.");

        x0        = std::max(region.tl().x(), region.x() + x0 - 3 * kExtraMargin);
        x1        = std::min(region.br().x(), region.x() + x1 + 3 * kExtraMargin);
        sec->bbox = {x0, region.y(), x1 - x0, region.height()};

        spdlog::debug("{:>{}} Detected x-section [{}--{}]", "+", level * 2, x0, x1);
        vsec->add_child_node(std::move(sec));
      }

      int new_level = level + 1;
      for (auto& sec : vsec->children)
      {
        spdlog::debug("{:<{}} Processing x-section [x={},w={}]", "", level * 2, sec->bbox.x, sec->bbox.width);
        sec->accept(*this, static_cast<void*>(&new_level));
      }
    }


    void visit(DOM::page* sec, void* extra)  final { this->vsplit(sec, *static_cast<int*>(extra)); }
    void visit(DOM::title_level_1*, void*)  final {}
    void visit(DOM::title_level_2*, void*) final {}
    void visit(DOM::section_level_1* sec, void* extra)  final { this->hsplit(sec, *static_cast<int*>(extra)); }
    void visit(DOM::section_level_2* sec, void* extra)  final { this->hsplit(sec, *static_cast<int*>(extra)); }
    void visit(DOM::column_level_1* sec, void* extra)  final { this->vsplit(sec, *static_cast<int*>(extra)); }
    void visit(DOM::column_level_2*, void*)  final { }
    void visit(DOM::entry*, void*)  final { }
    void visit(DOM::line*, void*)  final { }
  };



  ///
  /// Compute the real ROI of the image, ignoring black borders
  mln::box2d crop(const mln::image2d<uint8_t>& input)
  {
    using mln::box2d;
    using mln::point2d;

    int width = input.width();
    int height = input.height();


    box2d roi;

    // 1. Opening by super-large lines to crop the document to the good region
    {
      mln::image2d<uint8_t> vblock, hblock;
      // Detect left/right border
      {
        mln::se::periodic_line2d l(point2d{0, 1}, kLayoutPageOpeningHeight / 2);
        hblock = mln::morpho::opening(input, l);
        auto sum = sum_along_y_axis(hblock);
        auto [a,b] = detect_border(sum.data(), width, height);
        roi.tl().x() = a;
        roi.br().x() = b;
      }
      // Detect bottom/top border
      {
        {
          mln::se::periodic_line2d l(point2d{1, 0}, kLayoutPageOpeningWidth / 2);
          vblock = mln::morpho::opening(input, l);
        }
        mln::image2d<uint8_t> vblock2;
        // Opening with a vertical SE to connect lines (makes block)
        {
          mln::se::periodic_line2d l(point2d{0, 1}, kLayoutBlockOpeningHeight);
          vblock2 = mln::morpho::opening(vblock, l);
        }

        std::vector<int> sum = sum_along_x_axis(vblock2);
        auto [a, b]          = detect_border(sum.data(), height, width);
        roi.tl().y()         = a;
        roi.br().y()         = b;
      }

      if (kDebugLevel > 1)
      {
        mln::io::imsave(hblock, "tmp-1-page-h.tiff");
        mln::io::imsave(vblock, "tmp-1-page-v.tiff");
      }
      spdlog::debug("Document border (x1,y1,x2,y2): {} {} {} {}", roi.tl().x(), roi.tl().y(), roi.br().x(),
                    roi.br().y());
    }
    return roi;
  }

} // namespace






std::unique_ptr<DOMElement> DOMBlocksExtraction(ApplicationData* data)
{
  using mln::point2d;

  auto input0 = data->input;

  if (kDebugLevel > 1)
    mln::io::imsave(input0, "input0.tiff");

  auto roi = crop(input0);
  input0   = input0.clip(roi);

  // 2. Make blocks (connect letters/word and lines)
  mln::image2d<uint8_t> blocks;
  {
    // Opening with a vertical SE to connect lines (makes block)
    {
      mln::se::periodic_line2d l(point2d{0,1}, kLayoutBlockOpeningHeight / 2);
      blocks = mln::morpho::opening(input0, l);
    }
    // Opening with a horizontal SE to connect lines (makes block)
    {
      mln::se::periodic_line2d l(point2d{1,0}, kLayoutBlockOpeningWidth / 2);
      blocks = mln::morpho::opening(blocks, l);
    }
    if (kDebugLevel > 1)
      mln::io::imsave(blocks, "tmp-2-blocks.tiff");
  }


  // 3. Remove vertical line separator
  mln::image2d<uint8_t> blocks2;
  {
    mln::se::periodic_line2d l(point2d{1,0}, 3); // was 3
    using R = std::ratio<5,7>;
    blocks2 = mln::morpho::rank_filter<R>(blocks, l, mln::extension::bm::fill(uint8_t(255)));
    if (kDebugLevel > 1)
      mln::io::imsave(blocks2, "tmp-3-blocks.tiff");
  }

  data->blocks  = blocks;
  data->blocks2 = blocks2;


  // 4. Call ther recursive document segmentation
  std::unique_ptr<DOMElement> doc = DOMElement::create_document({roi.x(), roi.y(), roi.width(), roi.height()});
  DOMBlocksExtractor parser;

  auto& scaled_segments = data->segments;
  std::sort(scaled_segments.begin(), scaled_segments.end(), [](auto& s1, auto& s2) { return s1.start.y < s2.start.y; });
  parser.segments = scaled_segments;
  parser.blocks1 = std::move(blocks);
  parser.blocks2 = std::move(blocks2);

  int level = 0;
  doc->accept(parser, static_cast<void*>(&level));

  // Set to right scale (x2 for each coords because of the initial subsampling)
  // doc->scale(2);

  return doc;
}
