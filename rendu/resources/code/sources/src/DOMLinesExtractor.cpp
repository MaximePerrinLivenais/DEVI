#include "DOMLinesExtractor.hpp"

#include <mln/core/image/view/operators.hpp>
#include <mln/core/neighborhood/c4.hpp>
#include <mln/core/se/periodic_line2d.hpp>
#include <mln/core/se/rect2d.hpp>
#include <mln/io/imsave.hpp>
#include <mln/labeling/accumulate.hpp>
#include <mln/morpho/closing.hpp>
#include <mln/morpho/dynamic_filter.hpp>
#include <mln/morpho/opening.hpp>


#include "watershed.hpp"
#include "config.hpp"
#include "gaussian_directional_2d.hpp"


namespace
{

  int labelize_local_min_of_first_col(const mln::image2d<uint8_t>& input,
                                      mln::image2d<int16_t>& output,
                                      mln::box2d roi,
                                      int nlabel)
  {
    int y0 = roi.y();
    int x  = roi.x();
    int h  = roi.height();

    std::vector<int>     tmp0(h + 2);
    std::vector<uint8_t> tmp(h + 2, true);
    int*                 vals         = tmp0.data() + 1;
    bool*                is_local_min = (bool*)(tmp.data()) + 1;

    vals[-1] = 255;
    vals[h]  = 255;

    // Copy column
    for (int i = 0; i < h; ++i)
      vals[i] = input({x, y0 + i});

    // Forward
    for (int i = 0; i < h; ++i)
    {
      if (vals[i-1] < vals[i])
        is_local_min[i] = false;
      else if (vals[i-1] <= vals[i] && !is_local_min[i-1])
        is_local_min[i] = false;
    }

    // Backward
    for (int i = h - 1; i >= 0; --i)
    {
      if (vals[i+1] < vals[i])
        is_local_min[i] = false;
      else if (vals[i+1] <= vals[i] && !is_local_min[i+1])
        is_local_min[i] = false;
    }

    // Copy column
    is_local_min[-1] = false;
    for (int y = 0; y < h; ++y)
      if (is_local_min[y] && !is_local_min[y-1])
        output({x, y0 + y}) = ++nlabel;

    return nlabel;
  }

  struct BlurColumnVisitor : public DOMConstElementVisitor
  {
    const mln::image2d<uint8_t>* m_input;
    mln::image2d<uint8_t>*       m_output;
    mln::image2d<int16_t>*       m_line_markers;
    int                                        nlabel = 0;


    void visit(const DOM::page* e, void* extra) final { recurse(e, extra); }
    void visit(const DOM::title_level_1* e, void* extra) final { recurse(e, extra); };
    void visit(const DOM::title_level_2* e, void* extra) final { recurse(e, extra); };
    void visit(const DOM::section_level_1* e, void* extra) final { recurse(e, extra); };
    void visit(const DOM::section_level_2* e, void* extra) final { recurse(e, extra); };
    void visit(const DOM::column_level_1* e, void* extra) final { recurse(e, extra); };
    void visit(const DOM::entry*, void*) final{};
    void visit(const DOM::line*, void*) final{};

    void visit(const DOM::column_level_2* sec, void* extra) final;
  };


  void BlurColumnVisitor::visit(const DOM::column_level_2* sec, void*)
  {
    using mln::box2d;

    const box2d region(sec->bbox.x, sec->bbox.y, sec->bbox.width, sec->bbox.height);


    const float kLineVerticalSigma = (kLineHeight * 0.5f) * 0.1f;
    const float kLineHorizontalSigma = (kWordWidth * 0.5f) * 1.f;

    auto out = m_output->clip(region);
    mln::copy(m_input->clip(region), out);
    gaussian2d(out, kLineHorizontalSigma, kLineVerticalSigma, 255);


    // Labelize the minima
    nlabel = labelize_local_min_of_first_col(*m_output, *m_line_markers, region, nlabel);
  }


  struct bbox : mln::Accumulator<bbox>
  {
    using P = mln::point2d;
    using argument_type = mln::point2d;
    using result_type = mln::box2d;


    bbox() = default;
    void init() noexcept { *this = bbox(); }

    void take(P p)
    {
      xmin = std::min(xmin, p.x());
      xmax = std::max(xmax, p.x());
      ymin = std::min(ymin, p.y());
      ymax = std::max(ymax, p.y());
    }

    void take(bbox o)
    {
      xmin = std::min(xmin, o.xmin);
      xmax = std::max(xmax, o.xmax);
      ymin = std::min(ymin, o.ymin);
      ymax = std::max(ymax, o.ymax);
    }

    result_type to_result() const noexcept
    {
      if (xmin < xmax && ymin < ymax)
        return mln::box2d(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
      else
        return {};
    }


  private:
      int xmin = INT_MAX;
      int xmax = INT_MIN;
      int ymin = INT_MAX;
      int ymax = INT_MIN;
  };


  class DOMLineBuilder : public DOMElementVisitor
  {
    const mln::image2d<uint8_t>* m_input;
    const mln::image2d<int16_t>* m_ws;
    int                                        m_nlabel;

  public:
    DOMLineBuilder(const mln::image2d<uint8_t>& input, const mln::image2d<int16_t>& ws, int nlabel)
      : m_input(&input)
      , m_ws(&ws)
      , m_nlabel(nlabel)
    {
    }


    void visit(DOM::page* e, void* extra) final { recurse(e, extra); }
    void visit(DOM::title_level_1* e, void* extra) final { recurse(e, extra); };
    void visit(DOM::title_level_2* e, void* extra) final { recurse(e, extra); };
    void visit(DOM::section_level_1* e, void* extra) final { recurse(e, extra); };
    void visit(DOM::section_level_2* e, void* extra) final { recurse(e, extra); };
    void visit(DOM::column_level_1* e, void* extra) final { recurse(e, extra); };
    void visit(DOM::entry*, void*) final{/* throw std::logical_error("Unsupported op."); */};
    void visit(DOM::line*, void*) final{/* throw std::logical_error("Unsupported op."); */};
    void visit(DOM::column_level_2* sec, void* extra) final;
  };


  void DOMLineBuilder::visit(DOM::column_level_2* sec, void*)
  {
    using mln::box2d;
    using namespace mln::view::ops;

    const box2d region(sec->bbox.x, sec->bbox.y, sec->bbox.width, sec->bbox.height);

    //const float kLineVerticalSigma = (kLineHeight / 2.f) / 10.f;
    //const float kLineHorizontalSigma = (kWordWidth / 2.f);

    auto ws = m_ws->clip(region);
    auto input = m_input->clip(region);
    auto tmp = mln::view::ifelse(input < kLayoutWhiteLevel, ws, int16_t(0));
    auto boxes = mln::labeling::accumulate(tmp, m_nlabel, bbox{});

    for (int i = 1; i <= m_nlabel; ++i)
    {
      if (!boxes[i].empty())
      {
        auto node = std::make_unique<DOM::line>();

        node->label = i;
        node->bbox  = {boxes[i].x(), boxes[i].y(), boxes[i].width(), boxes[i].height()};
        sec->add_child_node(std::move(node));
      }
    }
  }

} // namespace


/// Detect the lines and stores them as new DOM::Line nodes
void DOMLinesExtraction(DOMElement* document, ApplicationData* data)
{
  using mln::point2d;

  // Opening with a horizontal SE to give matters to letters (merge letter/words but not lines)
  mln::image2d<uint8_t> f;
  {
    mln::se::periodic_line2d l(point2d{1,0}, kLayoutBlockOpeningWidth / 2);
    f = mln::morpho::opening(data->input, l);

    if (kDebugLevel > 1)
      mln::io::imsave(f, "ws-input.tiff");
  }

  mln::image2d<uint8_t> blurred;
  mln::resize(blurred, f).set_init_value(uint8_t(255));

  mln::image2d<int16_t> markers;
  mln::resize(markers, f).set_init_value(0);


  // 1. Blur the image in columns only and prepare WS markers
  BlurColumnVisitor viz;
  viz.m_input = &f;
  viz.m_output = &blurred;
  viz.m_line_markers = &markers;
  document->accept(viz, nullptr);

  if (kDebugLevel > 1)
    mln::io::imsave(blurred, "ws-blurred.tiff");

  // 2. Closing
  auto r   = mln::se::rect2d(static_cast<int>((kWordWidth / 2.f) + 0.5f),
                                           static_cast<int>((kLineHeight / 2.f) / 3.f + 0.5f));
  auto clo = mln::morpho::closing(blurred, r);

  if (kDebugLevel > 1)
    mln::io::imsave(clo, "ws-clo-1.tiff");

  constexpr int kClosingDynamic = 15;
  clo = mln::morpho::dynamic_closing(clo,  mln::c4, kClosingDynamic);

  if (kDebugLevel > 1)
    mln::io::imsave(clo, "ws-clo-2.tiff");


  // 3. Watershed transform

  if (kDebugLevel > 1)
    mln::io::imsave(markers, "markers.tiff");

  int  nlabel = viz.nlabel;
  impl::watershed(clo, mln::c4, markers, nlabel);

  auto ws = markers;

  if (kDebugLevel > 1)
    mln::io::imsave(ws, "ws.tiff");


  // 4. Compute the bounding box and insert lines
  {
    DOMLineBuilder viz(data->input, ws, nlabel);
    document->accept(viz, nullptr);
  }


  //auto wsup = upsample(ws, input.domain());
  data->ws = ws;
}
