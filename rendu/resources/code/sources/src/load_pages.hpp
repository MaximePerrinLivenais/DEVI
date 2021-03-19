#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "InternalTypes.hpp"

namespace poppler
{
  class document;
}

/// \brief Open the pdf document at location \p filename
/// Return null if the document cannot be opened
std::shared_ptr<poppler::document> open_document(const char* filename) noexcept;


/// \brief Load the page \p page from the document \p document
/// and returns its content (image + text boxes)
/// Return nullopt if the page is invalid
std::optional<PageData> load_page(poppler::document* document, int page) noexcept;
