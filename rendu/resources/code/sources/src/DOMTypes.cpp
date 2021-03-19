#include <DOMTypes.hpp>
#include <string>
#include <unordered_map>

void DOMElement::scale(float s)
{
  this->bbox.x *= s;
  this->bbox.y *= s;
  this->bbox.width *= s;
  this->bbox.height *= s;

  for (auto& sec : this->children)
    sec->scale(s);
}


std::unique_ptr<DOMElement> DOMElement::create_document(Box roi)
{
  auto p = std::make_unique<DOM::page>();
  p->bbox = roi;
  return p;
}


void DOMElementVisitor::recurse(DOMElement* e, void* extra)
{
  for (auto& child : e->children)
    child->accept(*this, extra);
}

void DOMConstElementVisitor::recurse(const DOMElement* e, void* extra)
{
  for (const auto& child : e->children)
    child->accept(*this, extra);
}

void DOMElement::add_child_node(std::unique_ptr<DOMElement> element)
{
  children.push_back(std::move(element));
}

bool DOMElement::has_children() const
{
  return !children.empty();
}



namespace
{
  const std::string enum2str[] = {
      "PAGE",            //
      "TITLE_LEVEL_1",   //
      "TITLE_LEVEL_2",   //
      "SECTION_LEVEL_1", //
      "SECTION_LEVEL_2", //
      "COLUMN_LEVEL_1",  //
      "COLUMN_LEVEL_2",  //
      "ENTRY",           //
      "LINE",            //
  };


  const std::unordered_map<std::string_view, DOMCategory> str2enum = {
    {"PAGE",            DOMCategory::PAGE            }, //
    {"TITLE_LEVEL_1",   DOMCategory::TITLE_LEVEL_1   }, //
    {"TITLE_LEVEL_2",   DOMCategory::TITLE_LEVEL_2   }, //
    {"SECTION_LEVEL_1", DOMCategory::SECTION_LEVEL_1 }, //
    {"SECTION_LEVEL_2", DOMCategory::SECTION_LEVEL_2 }, //
    {"COLUMN_LEVEL_1",  DOMCategory::COLUMN_LEVEL_1  }, //
    {"COLUMN_LEVEL_2",  DOMCategory::COLUMN_LEVEL_2  }, //
    {"ENTRY",           DOMCategory::ENTRY           }, //
    {"LINE",            DOMCategory::LINE            }, //
  };
}


std::unique_ptr<DOMElement> DOMElement::create_node(DOMCategory cat)
{
  // clang-format off
  switch (cat)
  {
  case DOMCategory::PAGE:              return std::make_unique<DOM::page>();
  case DOMCategory::TITLE_LEVEL_1:     return std::make_unique<DOM::title_level_1>();
  case DOMCategory::TITLE_LEVEL_2:     return std::make_unique<DOM::title_level_2>();
  case DOMCategory::SECTION_LEVEL_1:   return std::make_unique<DOM::section_level_1>();
  case DOMCategory::SECTION_LEVEL_2:   return std::make_unique<DOM::section_level_2>();
  case DOMCategory::COLUMN_LEVEL_1:    return std::make_unique<DOM::column_level_1>();
  case DOMCategory::COLUMN_LEVEL_2:    return std::make_unique<DOM::column_level_2>();
  case DOMCategory::ENTRY:             return std::make_unique<DOM::entry>();
  case DOMCategory::LINE:              return std::make_unique<DOM::line>();
  }
  // clang-format on
  return nullptr;
}

std::unique_ptr<DOMElement> DOMElement::create_node(std::string_view cat)
{
  return DOMElement::create_node(str2enum.at(cat));
}

std::string_view DOMElement::type_str() const
{
  return enum2str[(int)this->type()];
}




