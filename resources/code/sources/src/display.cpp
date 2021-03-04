#include "display.hpp"

#include "InternalTypes.hpp"

#include "region_lut.hpp"
#include <array>
#include <blend2d.h>
#include <mln/core/algorithm/for_each.hpp>
#include <mln/core/algorithm/transform.hpp>
#include <mln/core/image/view/zip.hpp>
#include <mln/core/range/view/zip.hpp>
#include <random>
#include <spdlog/spdlog.h>

namespace
{

  struct bgra_t
  {
    uint8_t b,g,r,a;
  };

  void labelize_line(const mln::image2d<uint8_t>* input_,  //
                     const mln::image2d<int16_t>* ws_,     //
                     mln::image2d<bgra_t>*        output_, //
                     const DOM::line* e, int entry_number, //
                     const display_options_t& opts)

  {
    mln::box2d region(e->bbox.x, e->bbox.y, e->bbox.width, e->bbox.height);

    auto in = input_->clip(region);
    auto out  = output_->clip(region);
    auto lbls = ws_->clip(region);

    mln::rgb8 c     = {0, 0, 0};
    int label = e->label;
    if (opts.show_lines == display_options_t::LINE_ENTRY)
      c = region_lut(entry_number);
    else if (opts.show_lines == display_options_t::LINE_INDENT)
      c = (e->indented) ? mln::rgb8{255, 0, 0} : mln::rgb8{0, 255, 0};
    else if (opts.show_lines == display_options_t::LINE_EOL)
      c = (e->reach_EOL) ? mln::rgb8{255, 0, 0} : mln::rgb8{0, 255, 0};
    else if (opts.show_lines == display_options_t::LINE_NUMBER)
      c = region_lut(label);

    bgra_t c2 = {c[2], c[1], c[0], 255};

    auto vals = mln::ranges::view::zip(in.values(), out.values(), lbls.values());
    for (auto&& r : mln::ranges::rows(vals))
      for (auto && [vin, vout, vlbl] : r)
      {
        if (vlbl == label && vin < 150)
          vout = c2;
      }
  }

  struct document_drawer : public DOMConstElementVisitor
  {

    display_options_t                          opts;
    const mln::image2d<uint8_t>* input;
    mln::image2d<bgra_t>*        output;
    const mln::image2d<int16_t>* ws;
    BLContext*                                 ctx;

    void visit(const DOM::page* e, void* extra) final
    {
      ctx->setStrokeStyle(BLRgba32(0u, 255u, 0u, 80u));
      ctx->setStrokeWidth(5);
      ctx->strokeBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
      this->recurse(e, extra);
    }

    void visit(const DOM::title_level_1* e, void*) final
    {
      ctx->setStrokeStyle(BLRgba32(255u, 0u, 0u, 255));
      ctx->setStrokeWidth(3);
      ctx->strokeBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
    }

    void visit(const DOM::title_level_2* e, void*) final
    {
      ctx->setStrokeStyle(BLRgba32(255u, 0u, 0u, 255));
      ctx->setStrokeWidth(3);
      ctx->strokeBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
    }

    void visit(const DOM::section_level_1* e, void* extra) final
    {
      ctx->setStrokeStyle(BLRgba32(0u, 0u, 255u, 255));
      ctx->setStrokeWidth(2);
      ctx->strokeLine(e->bbox.x - 100, e->bbox.y - 1, e->bbox.x1() + 100, e->bbox.y - 1);
      ctx->strokeLine(e->bbox.x - 100, e->bbox.y1() - 1, e->bbox.x1() + 100, e->bbox.y1() - 1);

      // ctx->setFillStyle(BLRgba32(255u, 0u, 0u, 25u));
      // ctx->fillBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
      this->recurse(e, extra);
    }

    void visit(const DOM::section_level_2* e, void* extra) final
    {
      ctx->setFillStyle(BLRgba32(255u, 0u, 0u, 25u));
      ctx->fillBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
      // ctx->setFillStyle(BLRgba32(0u, 255u, 0u, 25u));
      // ctx->fillBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
      this->recurse(e, extra);
    }

    void visit(const DOM::column_level_1* e, void* extra) final
    {
      this->recurse(e, extra);
    }


    void visit(const DOM::line* e, void* extra) final
    {
      if (opts.show_lines)
      {
        // auto c = region_lut(e->label);
        // ctx->setStrokeStyle(BLRgba32(c[0], c[1], c[2]));
        // ctx->setStrokeWidth(1);
        // ctx->strokeBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
        intptr_t k = (intptr_t)extra;
        labelize_line(input, ws, output, e, k, opts);
      }
      this->recurse(e, extra);
    }

    void visit(const DOM::entry* e, void* extra) final
    {
      /*
      if (!opts.hide_entries)
      {
        intptr_t k = (intptr_t)extra;
        auto c = region_lut(k);
        ctx->setStrokeStyle(BLRgba32(c[0], c[1], c[2]));
        ctx->setStrokeWidth(1);
        ctx->strokeBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
      }
      */
      this->recurse(e, extra);
    }


    void visit(const DOM::column_level_2* e, void*) final
    {
      ctx->setStrokeStyle(BLRgba32(0u, 0u, 255u, 100));
      ctx->setStrokeWidth(2);
      ctx->strokeBox(e->bbox.x, e->bbox.y, e->bbox.x1(), e->bbox.y1());
      intptr_t k = 1;
      for (auto& c : e->children)
      {
        c->accept(*this, (void*)k);
        k++;
      }
    }
  };




  void draw_ws_lines(const mln::image2d<int16_t>& lbls, mln::image2d<mln::rgb8>& out)
  {
    auto z = mln::view::zip(lbls, out);
    mln::for_each(z, [](const auto& v) {
      auto [lbl, vout] = v;
      if (lbl == 0)
        vout = mln::rgb8{0, 0, 0};
    });
  }
} // namespace


mln::image2d<mln::rgb8> //
display(DOMElement* document, ApplicationData* data, const display_options_t& opts)
{
  auto input = data->deskewed.image;
  auto lbls = data->ws;
  auto segments = data->deskewed.segments;

  assert(input.domain() == lbls.domain());

  auto out     = mln::transform(input, [](uint8_t x) -> bgra_t { return {x, x, x, 0}; });

  BLImage  img;
  BLResult err;
  int      width  = out.width();
  int      height = out.height();
  int      stride = out.byte_stride();
  spdlog::debug("w={} s={}", width, stride);
  err = img.createFromData(width, height, BL_FORMAT_PRGB32, out.buffer(), stride);
  if (err != BL_SUCCESS)
    spdlog::error("Unable to create rendering image.");


  BLContext ctx(img);
  err = ctx.setCompOp(BL_COMP_OP_SRC_OVER);
  if (err != BL_SUCCESS)
    spdlog::error("Unable to set comp op.");

  // Draw the grid
  if (opts.show_grid)
  {
    ctx.setStrokeStyle(BLRgba32(50, 50, 50));
    ctx.setStrokeWidth(1);
    for (int y = 0; y < height; y += 30)
      ctx.strokeLine(0, y, width - 1, y);
    for (int x = 0; x < width; x += 30)
      ctx.strokeLine(x, 0, x, height - 1);
  }

  // Draw the page
  if (opts.show_layout || opts.show_lines)
  {
    document_drawer drawer;
    drawer.opts   = opts;
    drawer.input  = &input;
    drawer.output = &out;
    drawer.ws     = &lbls;
    drawer.ctx    = &ctx;
    document->accept(drawer, nullptr);
  }

  // Draw segments
  if (opts.show_segments)
  {
    int k = 0;
    for (const auto& s : segments)
    {
      mln::rgb8 c = region_lut(k++);
      ctx.setStrokeStyle(BLRgba32(c[0], c[1], c[2]));
      ctx.setStrokeWidth(s.width);
      ctx.strokeLine(s.start.x, s.start.y, s.end.x, s.end.y);
    }
  }
  ctx.end();

  auto res = mln::transform(out, [](const bgra_t& x) -> mln::rgb8 { return {x.r, x.g, x.b}; });

  // Draw watershed lines
  if (opts.show_ws)
    draw_ws_lines(lbls, res);


  return res;
}
