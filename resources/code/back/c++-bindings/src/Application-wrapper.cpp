#include "Application-wrapper.hpp"
#include "DOMTypes-wrapper.hpp"

#include <Application.hpp>
#include <PDFInfo.hpp>
#include <pybind11/pybind11.h>
#include "ndimage_buffer_helper.hpp"
#include <mln/core/image/ndbuffer_image.hpp>


namespace py = pybind11;


class PyProgress : public Progress
{
public:
  void Update(int value) final { PYBIND11_OVERLOAD_PURE(void, Progress, Update, value); }
};


PyPDFInfo::PyPDFInfo(const std::string& filename)
{
  m_pdf = std::make_unique<PDFInfo>(filename);
}

int PyPDFInfo::get_num_pages() const
{
  return m_pdf->get_num_pages();
}


PyApplication::PyApplication(const std::string& uri, int page, Progress* progress = nullptr, bool deskew_only = false)
{
  py::gil_scoped_release release;
  m_app = std::make_unique<Application>(uri, page, progress, deskew_only);
}

PyApplication::~PyApplication()
{
}


py::array   PyApplication::GetInputImage() const
{
  py::array arr = mln::py::ndimage_to_buffer(m_app->GetInputImage());
  return arr;
}

py::array   PyApplication::GetDeskewedImage() const
{
  py::array arr = mln::py::ndimage_to_buffer(m_app->GetDeskewedImage());
  return arr;
}

py::object   PyApplication::GetDocument() const
{
  DOMElement* doc = m_app->GetDocument();
  return to_python(doc);
}

/*
void PyApplication::SetDocument(py::object doc)
{
  from_python(doc);
}
*/

PYBIND11_MODULE(soducocxx, m)
{
  py::class_<PyApplication>(m, "Application")
    .def(py::init<const std::string&, int, PyProgress*, bool>())
    .def_property_readonly("InputImage", &PyApplication::GetInputImage, ::py::return_value_policy::reference_internal)
    .def_property_readonly("DeskewedImage", &PyApplication::GetDeskewedImage, ::py::return_value_policy::reference_internal)
    .def("GetDocument", &PyApplication::GetDocument)
    //.def("SetDocument", &PyApplication::SetDocument)
    ;


  py::class_<Progress, PyProgress>(m, "Progress")
    .def(py::init<>())
    .def("Update", &Progress::Update)
    .def("Cancel", &Progress::Cancel);

  py::class_<PyPDFInfo>(m, "PDFInfo")
    .def(py::init<std::string>())
    .def("get_num_pages", &PyPDFInfo::get_num_pages);
}
