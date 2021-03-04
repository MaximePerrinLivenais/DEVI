#pragma once

#include <memory>

namespace poppler
{
  class document;
}

class PDFInfo
{
public:
  PDFInfo(const std::string& filename);

  // Get the number of pages of the pdf
  int get_num_pages() const;

private:
  std::shared_ptr<poppler::document> doc;
};

