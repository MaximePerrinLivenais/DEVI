#pragma once

#include <DOMTypes.hpp>
#include <CoreTypes.hpp>
#include <mln/core/image/ndimage_fwd.hpp>
#include <atomic>

struct ApplicationData;

class Progress
{
public:
  // Callback function when the program does a progress (in pourcent, range 0-100)
  virtual void Update(int value) = 0;

  // Return true if another thread a requested a cancel
  bool IsCanceled() const;

  // Notify a cancel action
  void Cancel();

private:
  std::atomic<bool> m_cancel;
};


class Application
{
public:
  Application(std::string uri, int page, Progress* progress, bool deskew_only = false);
  ~Application();


  // Get the input page as a 8-bits graylevel image
  mln::ndbuffer_image GetInputImage();

  // Get the deskewed input image as 8-bits graylevel image
  mln::ndbuffer_image GetDeskewedImage() const;

  // Return the root document object
  DOMElement*         GetDocument();


  // Return internal application data
  ApplicationData*    GetApplicationData();

private:
  std::unique_ptr<ApplicationData> m_app_data;
  std::unique_ptr<DOMElement>      m_document;
};
