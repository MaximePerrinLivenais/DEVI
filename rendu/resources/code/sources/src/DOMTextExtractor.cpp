#include "DOMTextExtractor.hpp"
#include "config.hpp"

#include <THST/RTree.h>
#include <string_view>
#include <spdlog/spdlog.h>

namespace
{
  struct tree_element_t
  {
    tree_element_t() = default;

    tree_element_t(const std::pair<Box, std::string>& data)
      : text{data.second}
    {
      y        = data.first.y;
      center.x = data.first.x + data.first.width / 2;
      center.y = data.first.y + data.first.height / 2;
    }

    union {
      int     coords[2];
      Point2D center;
    };

    int                 y;
    std::string_view    text;
    int                 anchor = 0;
  };

  struct Indexable
  {
    const int* min(const tree_element_t& e) { return e.coords; }
    const int* max(const tree_element_t& e) { return e.coords; }
  };

  using Tree_t = spatial::RTree<int, tree_element_t, 2, 8, 4, Indexable>;


  struct TextExtractorVisitor : public DOMElementVisitor
  {
    Tree_t* rtree;

    std::vector<tree_element_t> tmp;

    void extract_text(DOM::TextualElement* e, int block_base)
    {
      auto  b       = e->bbox;
      int   pmin[2] = {b.x, b.y};
      int   pmax[2] = {b.x1(), b.y1()};

      tmp.clear();
      rtree->query(spatial::within<2>(pmin, pmax), std::back_inserter(tmp));

      // Compute box anchors (top rounded to a grid line)
      std::for_each(tmp.begin(), tmp.end(),
                    [block_base](auto& e) { e.anchor = (int)std::round((e.y - block_base) / float(kLineHeight)); });

      std::sort(tmp.begin(), tmp.end(), [](const auto& a, const auto& b) {
        return std::make_pair(a.anchor, a.center.x) < std::make_pair(b.anchor, b.center.x);
      });

      spdlog::debug("New text element");
      for (const auto& m : tmp)
      {
        spdlog::debug("  A: {}, T:{}\n", m.anchor, m.text);
        e->text += m.text;
        e->text += " ";
      }
    }

    virtual void visit(DOM::page* e, void*) override { recurse(e, (void*)(intptr_t)e->bbox.y); };
    virtual void visit(DOM::title_level_1* e, void*) override { extract_text(e, e->bbox.y); };
    virtual void visit(DOM::title_level_2* e, void*) override { extract_text(e, e->bbox.y); };
    virtual void visit(DOM::section_level_1* e, void*) override { recurse(e, (void*)(intptr_t)e->bbox.y); };
    virtual void visit(DOM::section_level_2* e, void*) override { recurse(e, (void*)(intptr_t)e->bbox.y); };
    virtual void visit(DOM::column_level_1* e, void*) override { recurse(e, (void*)(intptr_t)e->bbox.y); };
    virtual void visit(DOM::column_level_2* e, void*) override { recurse(e, (void*)(intptr_t)e->bbox.y); };
    virtual void visit(DOM::entry* e, void* arg) override { recurse(e, (void*)(intptr_t)e->bbox.y); };
    virtual void visit(DOM::line* e, void* arg) override { extract_text(e, (intptr_t)arg); };
  };
}

/// Attach the text of the lines and stores them in DOM::Line nodes
void DOMTextExtraction(DOMElement* document, ApplicationData* data)
{

  Tree_t rtree;
  rtree.insert(std::begin(data->deskewed.texts), std::end(data->deskewed.texts));

  TextExtractorVisitor viz;
  viz.rtree = &rtree;


  document->accept(viz, nullptr);
}
