#include "DOMTypes-wrapper.hpp"

namespace py = pybind11;

namespace
{
  struct ToPythonVisitor : public DOMConstElementVisitor
  {

    py::dict create_element(const DOMElement* e, void* parent_id)
    {
      py::dict  self;
      int       id = m_id++;

      auto b          = e->bbox;
      self["id"]      = id;
      if (parent_id)
        self["parent"] = (int)(intptr_t)parent_id;
      self["type"] = e->type_str();
      self["box"]  = std::make_tuple(b.x, b.y, b.width, b.height);

      if (!e->children.empty())
        recurse(e, (void*)(intptr_t)id);

      return self;
    }

    void add_to_list(py::dict&& element) { m_elements.append(std::move(element)); }

    void add_element(const DOMElement* e, void* parent_id)
    {
      auto x = create_element(e, parent_id);
      add_to_list(std::move(x));
    } 


    void add_element(const DOM::TextualElement* e, void* parent_id)
    {
      auto x = create_element(e, parent_id);
      x["text"] = e->text;
      add_to_list(std::move(x));
    }

    void add_element(const DOM::line* e, void* parent_id)
    {
      auto x = create_element(e, parent_id);
      x["text"] = e->text;
      x["indented"] = e->indented;
      x["EOL"] = e->reach_EOL;
      add_to_list(std::move(x));
    }

    py::list get_root() const { return m_elements; }


    virtual void visit(const DOM::page* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::title_level_1* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::title_level_2* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::section_level_1* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::section_level_2* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::column_level_1* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::column_level_2* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::entry* e, void* parent_id) override { add_element(e, parent_id); };
    virtual void visit(const DOM::line* e, void* parent_id) override { add_element(e, parent_id); };

  private:
    py::list m_elements;
    int      m_id = 0x100; // Elements [0 - 255] are reserved
  };


  /*
  std::unique_ptr<DOMElement>  from_python_impl(py::handle e);

  struct FromPythonVisitor : public DOMElementVisitor
  {
    void add_element(DOMElement* out, py::dict& in)
    {
      py::tuple bbox   = in["box"];
      int       x      = py::cast<py::int_>(bbox[0]);
      int       y      = py::cast<py::int_>(bbox[1]);
      int       width  = py::cast<py::int_>(bbox[2]);
      int       height = py::cast<py::int_>(bbox[3]);

      out->bbox = {x, y, width, height};

      if (in.contains("children"))
      {
        py::list children = in["children"];
        for (auto&& c : children)
          out->children.push_back(from_python_impl(c));
      }
    }


    void add_element(DOM::TextualElement* out, py::dict& in)
    {
      out->text = py::cast<py::str>(in["text"]);
      add_element((DOMElement*)out, in);
    }

    virtual void visit(DOM::page* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::title_level_1* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::title_level_2* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::section_level_1* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::section_level_2* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::column_level_1* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::column_level_2* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::entry* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
    virtual void visit(DOM::line* out, void* in) override { add_element(out, *reinterpret_cast<py::dict*>(in)); };
  };

  std::unique_ptr<DOMElement>  from_python_impl(py::handle e_)
  {
    py::dict e = e_.cast<py::dict>();

    std::string type_str = py::cast<py::str>(e["type"]);

    auto x = DOMElement::create_node(type_str);
    FromPythonVisitor viz;
    x->accept(viz, (void*)&e);
    return x;
  }
  */

} // namespace


py::object to_python(DOMElement* doc)
{
  ToPythonVisitor viz;
  doc->accept(viz, nullptr);
  return viz.get_root();
}


/*
std::unique_ptr<DOMElement>  from_python(py::object doc)
{
  return from_python_impl(doc);
}
*/
