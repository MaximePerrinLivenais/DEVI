#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <memory>


class Application;
class Progress;
class PDFInfo;

class PyApplication
{
public:
  PyApplication(const std::string& uri, int page, Progress* progress, bool deskew_only);
  ~PyApplication();

  PyApplication(const PyApplication&) = delete;
  PyApplication& operator=(const PyApplication&) = delete;

  pybind11::array   GetInputImage() const;
  pybind11::array   GetDeskewedImage() const;


  // Return the root document object
  pybind11::object  GetDocument() const;
  void              SetDocument(pybind11::object obj);

private:
  std::unique_ptr<Application> m_app;
};


class PyPDFInfo

{
public:
  PyPDFInfo(const std::string& filename);
  int get_num_pages() const;

private:
  std::unique_ptr<PDFInfo> m_pdf;
};

