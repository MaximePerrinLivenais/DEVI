#include "DOMEntriesExtractor.hpp"

#include "config.hpp"
#include "DOMBuilder_helpers.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace
{

  struct DOMEntriesExtractor : public DOMElementVisitor
  {
    const mln::image2d<uint8_t>* input;
    e_force_indent               force_indent = FORCE_NONE;

    void visit(DOM::page* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::title_level_1* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::title_level_2* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::section_level_1* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::section_level_2* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::column_level_1* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::column_level_2* sec, void* extra) final;
    void visit(DOM::entry* sec, void* extra) final { this->recurse(sec, extra); }
    void visit(DOM::line* sec, void* extra) final { this->recurse(sec, extra); }
  };

  /*
  constexpr int kNFilter = 6;

  static constexpr float lw[3][kNFilter] = {-0.02755463, 4.5238338,  -3.2384014, -1.030055,   0.50755626, -0.6941753,
                                            -3.973867,   -6.4046874, -2.8687444, -0.55866617, 4.835583,   -4.723095,
                                            -2.4710116,  3.3544345,  1.7902824,  -3.3563163,  2.620387,   0.53217036};

  static constexpr float rw[3][kNFilter] = {1.020765,   2.2055726,   1.2751873,   -0.13907719, -0.7650537, 2.1323185,
                                            1.5263116,  -1.1369838,  -0.10114026, 1.6989342,   -1.9447949, -0.04686181,
                                            0.09115282, -0.14541107, 0.54059523,  -0.05082651, 0.07130285, 0.25157788};

  static constexpr float w2[3 * kNFilter] = {1.9570931,  -0.93785703, -0.4777993, 2.7529087,   -1.8349427, -0.10945565,
                                             2.0794728,  2.094579,    2.607956,   0.02096402,  -2.0741167, 2.746651,
                                             -0.8180196, -1.4098756,  1.6255764,  -0.08534542, 0.60639817, -0.03582852};

  static constexpr float b1[kNFilter] = {0.334649, 0.7842396, 0.31722787, 0.3184551, 0.43029153, 0.3035369};
  static constexpr float b2           = -1.0919825;


  float _4_conv_3x2(const float* lm, const float* rm, int i)
  {


    // First conv
    float f[3][kNFilter];
    for (int k = 0; k < kNFilter; ++k)
    {
      f[0][k] = lm[i - 2] * lw[0][k] + lm[i - 1] * lw[1][k] + lm[i] * lw[2][k] + //
                rm[i - 2] * rw[0][k] + rm[i - 1] * rw[1][k] + rm[i] * rw[2][k] + b1[k];

      f[1][k] = lm[i - 1] * lw[0][k] + lm[i] * lw[1][k] + lm[i+1] * lw[2][k] + //
                rm[i - 1] * rw[0][k] + rm[i] * rw[1][k] + rm[i+1] * rw[2][k] + b1[k];

      f[2][k] = lm[i] * lw[0][k] + lm[i+1] * lw[1][k] + lm[i + 2] * lw[2][k] + //
                rm[i] * rw[0][k] + rm[i+1] * rw[1][k] + rm[i + 2] * rw[2][k] + b1[k];

      f[0][k] = std::max(0.f, f[0][k]);
      f[1][k] = std::max(0.f, f[1][k]);
      f[2][k] = std::max(0.f, f[2][k]);
    }

    float* ravel = &f[0][0];
    float  out   = b2;
    for (int i = 0; i < 3 * kNFilter; ++i)
      out += ravel[i] * w2[i];
    out = 1 / (1 + std::exp(-out));
    return out;
  }

  void entry_predictor(const float* lm, const float* rm, std::size_t n, uint8_t* out)
  {
    for (std::size_t i = 0; i < n; ++i)
    {
      float pred  = _4_conv_3x2(lm, rm, i);
      out[i] = pred > 0.5;

      spdlog::debug("Prediction for line {} is {}", i, (int)out[i]);
    }
  }
  */


  float classify_0(float lspace_abs, float grad, float prspace)
  {
  if (prspace <= 0.050192078575491905) {
    if (grad <= 7.0) {
      if (lspace_abs <= 5.0) {
        return 0.2806137080585029;
      } else /* (lspace_abs > 5.0) */ {
        return 0.009406046080173251;
      }
    } else /* (grad > 7.0) */ {
      if (prspace <= 0.023809523321688175) {
        return 0.5186166688750498;
      } else /* (prspace > 0.023809523321688175) */ {
        return 0.8073099294583558;
      }
    }
  } else /* (prspace > 0.050192078575491905) */ {
    if (lspace_abs <= 15.0) {
      return 0.9956150117952923;
    } else /* (lspace_abs > 15.0) */ {
      if (grad <= 6.0) {
        return 0.287751801205705;
      } else /* (grad > 6.0) */ {
        if (grad <= 17.0) {
          return 0.8895712641566001;
        } else /* (grad > 17.0) */ {
          return 0.5709700948212982;
        }
      }
    }
  }
  }

  float classify_1(float lspace_abs, float grad, float prspace)
  {
  if (grad <= 5.0) {
    if (prspace <= 0.015286649111658335) {
      if (lspace_abs <= 1.0) {
        if (prspace <= 0.0012019231216982007) {
          return 0.23448713324461126;
        } else /* (prspace > 0.0012019231216982007) */ {
          return 0.5701066700938181;
        }
      } else /* (lspace_abs > 1.0) */ {
        return 0.7509394870522096;
      }
    } else /* (prspace > 0.015286649111658335) */ {
      if (lspace_abs <= 15.0) {
        return 0.9902016678266132;
      } else /* (lspace_abs > 15.0) */ {
        if (prspace <= 0.05302507430315018) {
          return 0.3559050064184852;
        } else /* (prspace > 0.05302507430315018) */ {
          return 0.9729120552826217;
        }
      }
    }
  } else /* (grad > 5.0) */ {
    if (prspace <= 0.05557460896670818) {
      return 0.00886907320584195;
    } else /* (prspace > 0.05557460896670818) */ {
      return 0.8568592722907037;
    }
  }
  }

  void entry_predictor(const float* lm, const float* rm, float colwidth, std::size_t n, uint8_t* out)
  {
    if (n == 0)
      return;


    std::vector<float>    proba_(2 * n);
    std::vector<uint8_t>  from_(2 * n);


    float*   proba[2] = {proba_.data(), proba_.data() + n};
    uint8_t* from[2]  = {from_.data(), from_.data() + n};


    proba[0][0] = 0.5f;
    proba[1][0] = 0.5f;

    for (std::size_t i = 1; i < n; ++i)
    {
      float lspace_abs = lm[i];
      float grad       = std::abs(lm[i] - lm[i - 1]);
      float prspace    = rm[i - 1] / colwidth;

      float p[2] = {
        classify_0(lspace_abs, grad, prspace),
        classify_1(lspace_abs, grad, prspace)
      };

      {
        float a = proba[0][i - 1] * p[0];
        float b = proba[1][i - 1] * p[1];
        from[1][i] = a > b ? 0 : 1;
        proba[1][i] = a > b ? a : b;
      }
      {
        float a = proba[0][i - 1] * (1-p[0]);
        float b = proba[1][i - 1] * (1-p[1]);
        from[0][i]  = a > b ? 0 : 1;
        proba[0][i] = a > b ? a : b;
      }
    }

    int lbl = proba[0][n - 1] > proba[1][n - 1] ? 0 : 1;
    for (std::size_t i = n - 1; i > 0; --i)
    {
      out[i] = lbl;
      spdlog::debug("Prediction for line {} is {} (p={})", i, lbl, proba[lbl][i]);
      lbl = from[lbl][i];
    }
    out[0] =  lbl;
  }



  void DOMEntriesExtractor::visit(DOM::column_level_2* sec, void*)
  {
    using mln::box2d;


    int ymin   = sec->bbox.y;
    int xmin   = sec->bbox.x;
    int xmax   = sec->bbox.x1();

    spdlog::debug("Start column x={}--{} y={} indent detection", xmin, xmax, ymin);


    std::size_t              nlines = sec->children.size();
    std::vector<uint8_t>        is_entry_start(nlines, false);

    // Compute the features
    {
      std::vector<float>       feat_buffer_1((nlines + 4), 0); // 2 rows for padding
      std::vector<float>       feat_buffer_2((nlines + 4), 0); // 2 rows for padding

      int                      colwidth = sec->bbox.width;
      int i = 2;
      for (auto& child : sec->children)
      {
        feat_buffer_1[i] = (child->bbox.x - xmin);  // float(colwidth);
        feat_buffer_2[i] = (xmax - child->bbox.x1()); // float(colwidth);
        i += 1;
      }


      entry_predictor(feat_buffer_1.data() + 2, feat_buffer_2.data() + 2, colwidth, nlines, is_entry_start.data());
    }


    auto entry = std::make_unique<DOM::entry>();
    auto dom_lines = std::move(sec->children);
    sec->children.clear();

    for (std::size_t i = 0; i < dom_lines.size(); ++i)
    {
      auto* l = dynamic_cast<DOM::line*>(dom_lines[i].get());
      l->indented = is_entry_start[i];

      if (is_entry_start[i] && entry->has_children())
      {
        // Start a new entry
        sec->children.push_back(std::move(entry));
        entry = std::make_unique<DOM::entry>();
      }
      entry->children.push_back(std::move(dom_lines[i]));
    }

    if (entry->has_children())
      sec->children.push_back(std::move(entry));

    // Compute bboxes
    for (auto& entry : sec->children)
    {
      Box b = entry->children[0]->bbox;
      for (std::size_t i = 1; i < entry->children.size(); i++)
        b.merge(entry->children[i]->bbox);
      entry->bbox = b;
    }
  }

} // namespace

/// Detect the entries by merging lines (bottom -> up)
void DOMEntriesExtraction(DOMElement* document, ApplicationData* data)
{
  DOMEntriesExtractor viz;
  viz.input = &(data->blocks);
  //viz.force_indent = force_indent;
  document->accept(viz, nullptr);
}
