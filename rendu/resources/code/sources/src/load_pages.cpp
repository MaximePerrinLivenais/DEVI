#include "load_pages.hpp"

#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-image.h>
#include <poppler-page-renderer.h>
#include <spdlog/spdlog.h>

std::shared_ptr<poppler::document> open_document(const char* filename) noexcept
{
  poppler::document* d = poppler::document::load_from_file(filename);
  if (d == nullptr)
  {
    spdlog::error("Unable to open the document '{}'", filename);
    return nullptr;
  }

  std::shared_ptr<poppler::document> doc(d);
  return doc;
}


std::optional<PageData> load_page(poppler::document* doc, int page) noexcept
{
  int page_count = doc->pages();
  if (page < 1 || page > page_count)
  {
    spdlog::error("Invalid requested page {} (must be in range {}-{})\n", page, 1, page_count + 1);
    return std::nullopt;
  }

  std::unique_ptr<poppler::page> pg(doc->create_page(page - 1)); // 0 based-index

  PageData pp;

  // Get text
  {
    auto boxes = pg->text_list();
    for (const poppler::text_box& box : boxes)
    {
      auto bbox = box.bbox();
      auto txt  = box.text().to_utf8();
      std::string txt_s(txt.data(), txt.size());

      Box b = {(int)bbox.x(), (int)bbox.y(), (int)bbox.width(), (int)bbox.height()};
      pp.texts.emplace_back(b, std::move(txt_s));
    }
  }

  // Get image
  {
    poppler::page_renderer pr;
    pr.set_image_format(poppler::image::format_gray8);
    auto img = pr.render_page(pg.get());

    bool           copy_data       = true;
    int            sizes[2]        = {img.width(), img.height()};
    std::ptrdiff_t byte_strides[2] = {sizeof(uint8_t), img.bytes_per_row()};

    pp.image = mln::image2d<uint8_t>::from_buffer((uint8_t*)img.const_data(), sizes, byte_strides, copy_data);
  }

  return pp;
}
