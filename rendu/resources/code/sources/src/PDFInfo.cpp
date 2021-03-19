#include <PDFInfo.hpp>

#include "load_pages.hpp"
#include <poppler-document.h>

// Use the open_document function from the "load_page.cpp" file
PDFInfo::PDFInfo(const std::string& filename)
{
  doc = open_document(filename.c_str());
  if (doc == nullptr)
    throw std::runtime_error("Invalid document (see logs)");
}

int PDFInfo::get_num_pages() const
{
  return doc->pages();
}

