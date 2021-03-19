#pragma once

#include <DOMTypes.hpp>
#include <pybind11/pybind11.h>

pybind11::object            to_python(DOMElement* document);
std::unique_ptr<DOMElement> from_python(pybind11::object document);
