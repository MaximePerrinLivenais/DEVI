// Command line interface for the project

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <Application.hpp>


#include "InternalTypes.hpp"
#include "display.hpp"
#include "config.hpp"

#include "DOMExport.hpp"

#include <mln/io/imsave.hpp>

int main(int argc, char** argv)
{
  std::string       pdf_path;
  std::string       out_path;
  std::string       json_path;
  int               page_number;
  int               debug        = 0;
  bool              deskew_only  = false;
  e_force_indent    force_indent = FORCE_NONE;
  display_options_t opts;

  {
    CLI::App app{"App description"};
    app.add_option("pdf", pdf_path, "Path to the input (PDF)")->required()->check(CLI::ExistingFile);

    app.add_option("output", out_path, "Path to the debug output image (JPG).")->required();

    app.add_option("-o", json_path, "Path to the output json file.");
    app.add_flag("--deskew-only", deskew_only, "Only perform the deskew");

    app.add_option("-p,--page", page_number, "Page to demat.")->required();

    app.add_flag("-d,--debug", debug, "Export tmp images.");

    app.add_flag("--show-grid", opts.show_grid, "Show/Hide the grid");
    app.add_flag("--show-ws", opts.show_ws, "Show/Hide the watershed lines");
    app.add_flag("--show-layout", opts.show_layout, "Show/Hide the layout lines");
    app.add_flag("--show-segments", opts.show_segments, "Show/Hide LSD segments");
    app.add_flag_function(
        "--force-left", [&force_indent](size_t) { force_indent = FORCE_LEFT; }, "Force left indent");
    app.add_flag_function(
        "--force-right", [&force_indent](size_t) { force_indent = FORCE_RIGHT; }, "Force right indent");

    using L_t = display_options_t::e_line_mode;

    std::vector<std::pair<std::string, L_t>> map{{"no", display_options_t::LINE_NO_COLOR},
                                                 {"entry", display_options_t::LINE_ENTRY},
                                                 {"indent", display_options_t::LINE_INDENT},
                                                 {"eol", display_options_t::LINE_EOL},
                                                 {"number", display_options_t::LINE_NUMBER}};
    app.add_option("--color-lines", opts.show_lines, "Colorize lines.")
        ->transform(CLI::CheckedTransformer(map, CLI::ignore_case));

    CLI11_PARSE(app, argc, argv);
  }

  if (debug)
  {
    if (debug > 1)
      spdlog::set_level(spdlog::level::level_enum::debug);
    else
      spdlog::set_level(spdlog::level::level_enum::info);
    kDebugLevel = debug;
  }



  Application app(pdf_path, page_number, nullptr, deskew_only);


  if (deskew_only)
  {
    ApplicationData* data = app.GetApplicationData();
    mln::io::imsave(data->deskewed.image, out_path);
    std::exit(0);
  }


  // Export Json
  if (!json_path.empty())
    DOMExport(app.GetDocument(), json_path);


  auto out = display(app.GetDocument(), app.GetApplicationData(), opts);
  mln::io::imsave(out, out_path);


  /*

  // 4. Line detection
  DOMLinesExtraction(document.get(), data);

  // 5. Entry detection
  DOMEntriesExtraction(document.get(), data, force_indent);

  // 6. Rescale data
  {
    document->scale(2);

    data.input = pp.image;
    data.ws = upsample(data.ws, pp.image.domain());
  }

  // 7. Extract text boxes to lines
  DOMTextExtraction(document.get(), pp.texts.data(), pp.texts.size());


  // 8. Export
  if (!json_path.empty())
    DOMExport(document.get(), json_path);


  */
}
