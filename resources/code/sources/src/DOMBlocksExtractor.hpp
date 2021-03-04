#pragma once

#include "InternalTypes.hpp"
#include <DOMTypes.hpp>


// Extract the top-level blocks (Sections + Columns + Sub-section + Sub-column)
std::unique_ptr<DOMElement> DOMBlocksExtraction(ApplicationData* data);
