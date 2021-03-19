#pragma once

#include <memory>
#include <vector>
#include <string_view>

#include <CoreTypes.hpp>

enum class DOMCategory
{
  PAGE,
  TITLE_LEVEL_1,
  TITLE_LEVEL_2,
  SECTION_LEVEL_1,
  SECTION_LEVEL_2,
  COLUMN_LEVEL_1,
  COLUMN_LEVEL_2,
  ENTRY,
  LINE,
  //WORD,
};


namespace DOM
{
  // Pseudo-section
  struct vertical;
  struct horizontal;

  struct page;
  struct title_level_1;
  struct title_level_2;
  struct section_level_1;
  struct section_level_2;
  struct column_level_1;
  struct column_level_2;
  struct entry;
  struct line;

} // namespace DOM


class DOMElementVisitor;
class DOMConstElementVisitor;


class DOMElement
{
protected:
  /// Constructor made private
  DOMElement() = default;

public:
  DOMElement(DOMElement&&) = default;
  DOMElement& operator= (DOMElement&&) = default;


  /// Factory
  /// \{
  static std::unique_ptr<DOMElement> create_document(Box roi);
  static std::unique_ptr<DOMElement> create_node(DOMCategory cat);
  static std::unique_ptr<DOMElement> create_node(std::string_view cat);
  /// \}

  /// Modifiers
  /// \{
  /// Scale the bounding box by the given factor
  void scale(float s);
  /// \}


  /// Visitors
  /// \{
  virtual void accept(DOMConstElementVisitor& vis, void* extra) const = 0;
  virtual void accept(DOMElementVisitor& vis, void* extra) = 0;
  /// \}


  /// DOM Modifiers
  ///  \{
  //// Take ownership of the pointer and add it to the DOM
  void add_child_node(std::unique_ptr<DOMElement> element);
  /// \}

  /// DOM Queries
  ///  \{
  bool has_children() const;
  /// \}

  virtual DOMCategory      type() const = 0;
  std::string_view         type_str() const;

public:
  Box                                      bbox;
  std::vector<std::unique_ptr<DOMElement>> children;

private:
  friend class DOMElementVisitor;
  friend class DOMConstElementVisitor;
};




class DOMElementVisitor
{
public:
  void recurse(DOMElement* e, void* extra);
  virtual void visit(DOM::page* e, void* extra) = 0;
  virtual void visit(DOM::title_level_1* e, void* extra) = 0;
  virtual void visit(DOM::title_level_2* e, void* extra) = 0;
  virtual void visit(DOM::section_level_1* e, void* extra) = 0;
  virtual void visit(DOM::section_level_2* e, void* extra) = 0;
  virtual void visit(DOM::column_level_1* e, void* extra) = 0;
  virtual void visit(DOM::column_level_2* e, void* extra) = 0;
  virtual void visit(DOM::entry* e, void* extra) = 0;
  virtual void visit(DOM::line* e, void* extra) = 0;
};

class DOMConstElementVisitor
{
public:
  void recurse(const DOMElement* e, void* extra);
  virtual void visit(const DOM::page* e, void* extra) = 0;
  virtual void visit(const DOM::title_level_1* e, void* extra) = 0;
  virtual void visit(const DOM::title_level_2* e, void* extra) = 0;
  virtual void visit(const DOM::section_level_1* e, void* extra) = 0;
  virtual void visit(const DOM::section_level_2* e, void* extra) = 0;
  virtual void visit(const DOM::column_level_1* e, void* extra) = 0;
  virtual void visit(const DOM::column_level_2* e, void* extra) = 0;
  virtual void visit(const DOM::entry* e, void* extra) = 0;
  virtual void visit(const DOM::line* e, void* extra) = 0;
};


namespace DOM
{
  struct TextualElement : DOMElement
  {
    std::string text;
  };


  struct page : DOMElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::PAGE; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct title_level_1 : TextualElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::TITLE_LEVEL_1; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct title_level_2 : TextualElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::TITLE_LEVEL_2; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };


  struct section_level_1 : DOMElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::SECTION_LEVEL_1; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct section_level_2 : DOMElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::SECTION_LEVEL_2; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct column_level_1 : DOMElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::COLUMN_LEVEL_1; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct column_level_2 : DOMElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::COLUMN_LEVEL_2; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct entry : TextualElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::ENTRY; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }
  };

  struct line : TextualElement
  {
    virtual DOMCategory      type() const final { return DOMCategory::LINE; }
    virtual void             accept(DOMElementVisitor& vis, void* extra) final { vis.visit(this, extra); }
    virtual void             accept(DOMConstElementVisitor& vis, void* extra) const final { vis.visit(this, extra); }

    int  label;
    bool indented;
    bool reach_EOL;
  };


} // namespace sections
