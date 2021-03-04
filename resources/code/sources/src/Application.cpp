#include <Application.hpp>

#include <exception>

#include "load_pages.hpp"
#include "detect_separators.hpp"
#include "deskew.hpp"
#include "subsample.hpp"

#include "DOMBlocksExtractor.hpp"
#include "DOMLinesExtractor.hpp"
#include "DOMEntriesExtractor.hpp"
#include "DOMTextExtractor.hpp"

#include <mln/io/imsave.hpp>
#include <spdlog/spdlog.h>
#include "timer.hpp"
#include "config.hpp"

bool Progress::IsCanceled() const
{
  return m_cancel.load();
}

void Progress::Cancel()
{
  m_cancel.store(true);
}



// \brief Try to detect the scale of an image
// scale 0 = normal scale
// scale 1 = image / 2
//
// \return the scale or INT_MAX if the scale cannot be detected
int detect_scale(int width)
{
  float s = std::log2(2048.f / width);
  float rs = std::round(s);
  if (std::abs(s - rs) > 0.2f)
    return INT_MAX;
  return static_cast<int>(rs);
}

void handle_and_update_scale(int& scale, int width, int /*height*/)
{
  if (scale == INT_MAX)
    scale = detect_scale(width);
  if (scale == INT_MAX)
  {
    spdlog::error("The scale cannot be properly detected. It is set to 0.");
    scale = 0;
  }
  else if (scale != 0 && scale != 1)
  {
    spdlog::error("The scale {} is not handled. Running with scale = 0.", scale);
    scale = 0;
  }
  else
    spdlog::info("Running at scale={}.", scale);
}


Application::Application(std::string uri, int page_number, Progress* progress, bool deskew_only)
{
  const char* time_str = "'{}' computed in {:} ms";
  int scale = INT_MAX;

  m_app_data = std::make_unique<ApplicationData>();

  //spdlog::set_level(spdlog::level::level_enum::debug);
  //kDebugLevel = 2;

  clocker c;
  // Load the document and the page
  {
    c.restart();
    auto doc = open_document(uri.c_str());
    if (doc == nullptr)
      throw std::runtime_error("Invalid document (see logs)");
    auto   pp_ = load_page(doc.get(), page_number);
    if (!pp_)
      throw std::runtime_error("Invalid page (see logs)");

    spdlog::info("Document load (size={}x{}) computed in {:} ms", pp_->image.width(), pp_->image.height(),
                 c.GetElapsedTimeMilliSeconds());

    handle_and_update_scale(scale, pp_->image.width(), pp_->image.height());
    m_app_data->original = std::move(pp_.value());
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(10);

  // 1. Detect the segments
  {
    c.restart();
    m_app_data->original.segments = detect_separators(m_app_data->original.image);
    spdlog::info(time_str, "Segments computation", c.GetElapsedTimeMilliSeconds());
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(20);

  // 2. deskew document
  {
    c.restart();
    m_app_data->deskewed = deskew(m_app_data->original);
    spdlog::info(time_str, "Document deskew", c.GetElapsedTimeMilliSeconds());
  }

  if (deskew_only)
  {
    if (progress)
      progress->Update(100);
    return;
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(30);


  // 3. Subsample input
  {
    c.restart();
    if (scale == 0)
    {
      m_app_data->input = subsample(m_app_data->deskewed.image);
      m_app_data->segments = m_app_data->deskewed.segments;
      auto& segs = m_app_data->segments;
      std::for_each(segs.begin(), segs.end(), [](auto& seg) { seg.scale(0.5f); });

      spdlog::info(time_str, "Subsampling", c.GetElapsedTimeMilliSeconds());
    }
    else if (scale == 1)
    {
      m_app_data->input    = m_app_data->deskewed.image;
      m_app_data->segments = m_app_data->deskewed.segments;
    }
  }

  // 4. Block detection
  {
    c.restart();
    m_document = DOMBlocksExtraction(m_app_data.get());
    spdlog::info(time_str, "Blocks detection", c.GetElapsedTimeMilliSeconds());
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(50);


  // 5. Line detection
  {
    c.restart();
    DOMLinesExtraction(m_document.get(), m_app_data.get());
    spdlog::info(time_str, "Lines detection", c.GetElapsedTimeMilliSeconds());
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(60);

  // 6. Rescale data
  {
    c.restart();

    if (scale == 0)
    {
      m_document->scale(2);
      m_app_data->ws = upsample(m_app_data->ws, m_app_data->deskewed.image.domain());
      spdlog::info(time_str, "Upsampling", c.GetElapsedTimeMilliSeconds());
    }
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(70);

  // 6. Entry detection
  {
    c.restart();
    DOMEntriesExtraction(m_document.get(), m_app_data.get());
    spdlog::info(time_str, "Entries detection", c.GetElapsedTimeMilliSeconds());
  }

  if (progress && progress->IsCanceled())
    return;
  if (progress)
    progress->Update(80);




  // 7. Extract text boxes to lines
  {
    c.restart();
    DOMTextExtraction(m_document.get(), m_app_data.get());
    spdlog::info(time_str, "Text extraction", c.GetElapsedTimeMilliSeconds());
  }

  if (progress)
    progress->Update(100);
}


Application::~Application()
{
}

ApplicationData* Application::GetApplicationData()
{
  return m_app_data.get();
}

DOMElement* Application::GetDocument()
{
  return m_document.get();
}

// Get the input page as a 8-bits graylevel image
mln::ndbuffer_image  Application::GetInputImage()
{
  return m_app_data->original.image;
}

// Get the deskewed input image as 8-bits graylevel image
mln::ndbuffer_image Application::GetDeskewedImage() const
{
  return m_app_data->deskewed.image;
}

