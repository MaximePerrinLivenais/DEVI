#include "DOMTextExtractor.hpp"
#include "config.hpp"

#include <string_view>
#include <spdlog/spdlog.h>
#include <tesseract/baseapi.h>

namespace
{
  class TextExtractorVisitor : public DOMElementVisitor
  {
  private:
    tesseract::TessBaseAPI m_api;

  public:

    TextExtractorVisitor(ApplicationData* data)
      {

        if (m_api.Init(NULL, "fra")) {
          spdlog::error("Could not initialize tesseract.");
          throw std::runtime_error("Could not initialize tesseract");
        }
        else
          spdlog::info("Tesseract has been initialized.");

        auto ima = data->deskewed.image;
        m_api.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
        m_api.SetImage(ima.buffer(), ima.width(), ima.height(), ima.byte_stride(0), ima.byte_stride(1));
        m_api.SetSourceResolution(150);
      }

    ~TextExtractorVisitor() { m_api.End(); }


    void extract_text(DOM::TextualElement* e)
    {
      if (!e->bbox.empty())
      {
        spdlog::debug("Start extraction of rect (x={},y={},w={},h={})", e->bbox.x, e->bbox.y, e->bbox.width,
                      e->bbox.height);
        m_api.SetRectangle(e->bbox.x, e->bbox.y, e->bbox.width, e->bbox.height);

        char* txt = m_api.GetUTF8Text();
        e->text.assign(txt);
        delete[] txt;
      }
    }


    virtual void visit(DOM::page* e, void*) override { recurse(e, nullptr); };
    virtual void visit(DOM::title_level_1* e, void*) override { extract_text(e); };
    virtual void visit(DOM::title_level_2* e, void*) override { extract_text(e); };
    virtual void visit(DOM::section_level_1* e, void*) override { recurse(e, nullptr); };
    virtual void visit(DOM::section_level_2* e, void*) override { recurse(e, nullptr); };
    virtual void visit(DOM::column_level_1* e, void*) override { recurse(e, nullptr); };
    virtual void visit(DOM::column_level_2* e, void*) override { recurse(e, nullptr); };
    virtual void visit(DOM::entry* e, void*) override { extract_text(e); };
    virtual void visit(DOM::line*, void*) override { ; };
  };
} // namespace

/// Attach the text of the entries and stores them in DOM::Entries/  nodes
void DOMTextExtraction(DOMElement* document, ApplicationData* data)
{
  TextExtractorVisitor viz(data);
  document->accept(viz, nullptr);
}
