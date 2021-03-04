#include "DOMExport.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

namespace
{
  // for convenience
  using json = nlohmann::json;

  class JSonExportVisitor : public DOMConstElementVisitor
  {
    json* add_entry_base(const DOMElement* e, void* parent_id)
    {
      json  element;

      auto b          = e->bbox;
      int  id         = m_id++;
      element["id"]   = id;

      if (parent_id)
        element["parent"] = (int)(intptr_t)parent_id;
      element["type"] = e->type_str();
      element["box"]  = {b.x, b.y, b.width, b.height};

      if (!e->children.empty())
        recurse(e, (void*)(intptr_t)id);

      m_elements.push_back(std::move(element));
      return &m_elements.back();
    }

    void add_entry(const DOMElement* e, void* parent_id)
    {
      add_entry_base(e, parent_id);
    }

    void add_entry(const DOM::TextualElement* e, void* parent_id)
    {
      json* element       = add_entry_base(e, parent_id);
      (*element)["text"] = e->text;
    }

  public:
    JSonExportVisitor() = default;


    virtual void visit(const DOM::page* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::title_level_1* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::title_level_2* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::section_level_1* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::section_level_2* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::column_level_1* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::column_level_2* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::entry* e, void* parent_id) override { add_entry(e, parent_id); };
    virtual void visit(const DOM::line* e, void* parent_id) override { add_entry(e, parent_id); };


    json get_root() const { return m_elements; }


  private:
    json        m_elements = json::array();
    int         m_id = 0x100; // Elements 0-256 are reserved
  };
} // namespace


void DOMExport(const DOMElement* doc, const std::string& path)
{

  JSonExportVisitor viz;
  doc->accept(viz, nullptr);

  std::ofstream fs;
  fs.open(path);
  fs << viz.get_root();
}
