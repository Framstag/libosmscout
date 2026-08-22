/*
  SymbolsAll - a demo program for libosmscout
  Copyright (C) 2026  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
  Renders every symbol of a stylesheet to standalone image files so that
  all symbol definitions (e.g. AI-generated ones) can be visually scanned
  for correctness.

  Usage (executed from the build directory):

    Demos/SymbolsAll --stylesheet ../stylesheets/motorways.oss --output symbols-output
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <osmscout/TypeConfig.h>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Magnification.h>

#include <osmscoutmap/StyleConfig.h>

#if defined(HAVE_OSMSCOUT_MAP_CAIRO)
  #if defined(__WIN32__) || defined(WIN32)
    #include <cairo.h>
  #elif defined(__APPLE__) && __APPLE__
    #include <cairo.h>
  #else
    #include <cairo/cairo.h>
  #endif
  #include <osmscoutmapcairo/SymbolRendererCairo.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_SVG)
  #include <osmscoutmapsvg/SymbolRendererSVG.h>
#endif

namespace {

  enum class Backend {
    Cairo,
    Svg,
    All
  };

  struct Arguments
  {
    bool        help=false;
    bool        list=false;
    bool        sheet=false;
    std::string stylesheet;
    std::string output{"symbols-output"};
    std::string ost;
    std::string backendStr{"all"};
    double      dpi=96.0;
    size_t      size=256;
  };

  class SymbolsAllArgParser : public osmscout::CmdLineParser
  {
  private:
    Arguments args;

  public:
    SymbolsAllArgParser(const std::string& appName,
                        int argc,
                        char* argv[])
    : osmscout::CmdLineParser(appName, argc, argv)
    {
      AddOption(osmscout::CmdLineFlag([this](const bool&) {
                                        args.help=true;
                                      }),
                "help",
                "Show this help",
                true);
      AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                args.stylesheet=value;
                                              }),
                "stylesheet",
                "Stylesheet file to scan",
                false);
      AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                args.output=value;
                                              }),
                "output",
                "Output directory (" + args.output + ")",
                false);
      AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                args.ost=value;
                                              }),
                "ost",
                "Type definition file (default: sibling .ost or map.ost)",
                false);
      AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                args.backendStr=value;
                                              }),
                "backend",
                "Rendering backend: cairo, svg or all (" + args.backendStr + ")",
                false);
      AddOption(osmscout::CmdLineDoubleOption([this](const double& value) {
                                                args.dpi=value;
                                              }),
                "dpi",
                "Rendering DPI (" + std::to_string(args.dpi) + ")",
                false);
      AddOption(osmscout::CmdLineSizeTOption([this](const size_t& value) {
                                               args.size=value;
                                             }),
                "size",
                "Square canvas size in pixel (" + std::to_string(args.size) + ")",
                false);
      AddOption(osmscout::CmdLineFlag([this](const bool&) {
                                        args.list=true;
                                      }),
                "list",
                "Only print the names of all symbols",
                false);
      AddOption(osmscout::CmdLineFlag([this](const bool&) {
                                        args.sheet=true;
                                      }),
                "sheet",
                "Additionally render a contact sheet image",
                false);
    }

    Arguments GetArguments() const
    {
      return args;
    }
  };

  bool ParseBackend(const std::string& value,
                    Backend& backend)
  {
    if (value=="cairo") {
      backend=Backend::Cairo;
    }
    else if (value=="svg") {
      backend=Backend::Svg;
    }
    else if (value=="all") {
      backend=Backend::All;
    }
    else {
      return false;
    }

    return true;
  }

  /**
   * Resolve the type definition file to use: --ost, map.ost next to the
   * stylesheet or a sibling .ost of the stylesheet.
   */
  std::string ResolveOSTFile(const Arguments& args)
  {
    if (!args.ost.empty()) {
      return args.ost;
    }

    std::filesystem::path stylesheet(args.stylesheet);
    std::filesystem::path mapOst=stylesheet.parent_path() / "map.ost";

    if (std::filesystem::exists(mapOst)) {
      return mapOst.string();
    }

    std::filesystem::path sibling=stylesheet;

    sibling.replace_extension(".ost");

    if (std::filesystem::exists(sibling)) {
      return sibling.string();
    }

    return {};
  }

  /**
   * Replace all characters not valid in file names
   */
  std::string SanitizeFileName(const std::string& name)
  {
    std::string result;

    result.reserve(name.size());
    for (char c : name) {
      if (std::isalnum(static_cast<unsigned char>(c)) || c=='-' || c=='_') {
        result.push_back(c);
      }
      else {
        result.push_back('_');
      }
    }

    return result;
  }

  /**
   * Scale factor that fits the symbol (including border) into the canvas
   * with a small margin
   */
  double ComputeScaleFactor(const osmscout::Projection& projection,
                            const osmscout::Symbol& symbol,
                            size_t size)
  {
    osmscout::ScreenBox bbox=symbol.GetBoundingBox(projection);
    double              margin=symbol.GetMaxBorderWidth(projection)+8.0;
    double              usable=size>2*margin ? size-margin : size*0.75;
    double              scale=std::min(usable/std::max(bbox.GetWidth(),1.0),
                                       usable/std::max(bbox.GetHeight(),1.0));

    return std::max(scale,0.05);
  }

#if defined(HAVE_OSMSCOUT_MAP_CAIRO)

  /**
   * Render a single symbol via the Cairo backend into a PNG file
   */
  bool RenderSymbolCairo(const osmscout::Projection& projection,
                         const osmscout::Symbol& symbol,
                         const std::string& path,
                         size_t size)
  {
    cairo_surface_t * surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                         static_cast<int>(size),
                                                         static_cast<int>(size));

    if (surface==nullptr ||
        cairo_surface_status(surface)!=CAIRO_STATUS_SUCCESS) {
      return false;
    }

    cairo_t * cr=cairo_create(surface);

    cairo_set_source_rgb(cr,1.0,1.0,1.0);
    cairo_paint(cr);

    double                        scale=ComputeScaleFactor(projection,symbol,size);

    osmscout::SymbolRendererCairo renderer(cr);

    renderer.Render(projection,
                    symbol,
                    osmscout::Vertex2D(static_cast<double>(size)/2.0,
                                       static_cast<double>(size)/2.0),
                    scale);

    bool ok=cairo_surface_write_to_png(surface,path.c_str())==CAIRO_STATUS_SUCCESS;

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return ok;
  }

  /**
   * Render a contact sheet of all symbols via the Cairo backend
   */
  bool RenderSheetCairo(const osmscout::Projection& projection,
                        const osmscout::StyleConfig& styleConfig,
                        const std::vector<std::string>& names,
                        const std::string& path,
                        size_t size)
  {
    const size_t    labelHeight=24;

    size_t          count=names.size();
    size_t          cols=static_cast<size_t>(std::max(1.0,std::ceil(std::sqrt(static_cast<double>(count)))));
    size_t          rows=count==0 ? 1 : (count+cols-1)/cols;
    size_t          cellHeight=size+labelHeight;
    size_t          width=cols*size;
    size_t          height=rows*cellHeight;

    cairo_surface_t * surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                         static_cast<int>(width),
                                                         static_cast<int>(height));

    if (surface==nullptr ||
        cairo_surface_status(surface)!=CAIRO_STATUS_SUCCESS) {
      return false;
    }

    cairo_t * cairo=cairo_create(surface);

    cairo_select_font_face(cairo,
                           "sans",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo,12.0);

    for (size_t i=0; i<count; ++i) {
      size_t col=i%cols;
      size_t row=i/cols;
      double x=static_cast<double>(col*size);
      double y=static_cast<double>(row*cellHeight);

      cairo_save(cairo);
      cairo_translate(cairo,x,y);

      cairo_set_source_rgb(cairo,1.0,1.0,1.0);
      cairo_paint(cairo);

      osmscout::SymbolRef           symbol=styleConfig.GetSymbol(names[i]);
      double                        scale=ComputeScaleFactor(projection,*symbol,size);
      osmscout::SymbolRendererCairo renderer(cairo);

      renderer.Render(projection,
                      *symbol,
                      osmscout::Vertex2D(static_cast<double>(size)/2.0,
                                         static_cast<double>(size)/2.0),
                      scale);

      cairo_set_source_rgb(cairo,0.0,0.0,0.0);
      cairo_move_to(cairo,5.0,static_cast<double>(size)+16.0);
      cairo_show_text(cairo,names[i].c_str());

      cairo_restore(cairo);
    }

    bool ok=cairo_surface_write_to_png(surface,path.c_str())==CAIRO_STATUS_SUCCESS;

    cairo_destroy(cairo);
    cairo_surface_destroy(surface);

    return ok;
  }

#endif

#if defined(HAVE_OSMSCOUT_MAP_SVG)

  /**
   * Render a single symbol via the SVG backend into an SVG file
   */
  bool RenderSymbolSVG(const osmscout::Projection& projection,
                       const osmscout::Symbol& symbol,
                       const std::string& path,
                       size_t size)
  {
    std::ofstream stream(path);

    if (!stream) {
      return false;
    }

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    stream << "<svg xmlns=\"http://www.w3.org/2000/svg\""
           << " width=\"" << size << "\""
           << " height=\"" << size << "\""
           << " viewBox=\"0 0 " << size << " " << size << "\">" << std::endl;
    stream << "  <rect x=\"0\" y=\"0\" width=\"" << size << "\" height=\"" << size
           << "\" fill=\"white\"/>" << std::endl;

    double                      scale=ComputeScaleFactor(projection,symbol,size);
    osmscout::SymbolRendererSVG renderer(stream);

    renderer.Render(projection,
                    symbol,
                    osmscout::Vertex2D(static_cast<double>(size)/2.0,
                                       static_cast<double>(size)/2.0),
                    scale);

    stream << "</svg>" << std::endl;

    return stream.good();
  }

  /**
   * Render a contact sheet of all symbols into an SVG file
   */
  bool RenderSheetSVG(const osmscout::Projection& projection,
                      const osmscout::StyleConfig& styleConfig,
                      const std::vector<std::string>& names,
                      const std::string& path,
                      size_t size)
  {
    const size_t  labelHeight=24;

    size_t        count=names.size();
    size_t        cols=static_cast<size_t>(std::max(1.0,std::ceil(std::sqrt(static_cast<double>(count)))));
    size_t        rows=count==0 ? 1 : (count+cols-1)/cols;
    size_t        cellHeight=size+labelHeight;
    size_t        width=cols*size;
    size_t        height=rows*cellHeight;

    std::ofstream stream(path);

    if (!stream) {
      return false;
    }

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    stream << "<svg xmlns=\"http://www.w3.org/2000/svg\""
           << " width=\"" << width << "\""
           << " height=\"" << height << "\""
           << " viewBox=\"0 0 " << width << " " << height << "\">" << std::endl;
    stream << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
           << "\" fill=\"white\"/>" << std::endl;

    for (size_t i=0; i<count; ++i) {
      size_t col=i%cols;
      size_t row=i/cols;
      double x=static_cast<double>(col*size);
      double y=static_cast<double>(row*cellHeight);

      stream << "  <g transform=\"translate(" << x << "," << y << ")\">" << std::endl;

      osmscout::SymbolRef         symbol=styleConfig.GetSymbol(names[i]);
      double                      scale=ComputeScaleFactor(projection,*symbol,size);
      osmscout::SymbolRendererSVG renderer(stream);

      renderer.Render(projection,
                      *symbol,
                      osmscout::Vertex2D(static_cast<double>(size)/2.0,
                                         static_cast<double>(size)/2.0),
                      scale);

      stream << "  </g>" << std::endl;
      stream << "  <text x=\"" << x+5.0 << "\" y=\"" << y+static_cast<double>(size)+16.0
             << "\" font-family=\"sans-serif\" font-size=\"12\" fill=\"black\">"
             << names[i] << "</text>" << std::endl;
    }

    stream << "</svg>" << std::endl;

    return stream.good();
  }

#endif
}

int main(int argc, char* argv[])
{
  SymbolsAllArgParser          argParser("SymbolsAll", argc, argv);
  osmscout::CmdLineParseResult argResult=argParser.Parse();

  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;

    return 1;
  }

  Arguments args=argParser.GetArguments();

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;

    return 0;
  }

  if (args.stylesheet.empty()) {
    std::cerr << "ERROR: No stylesheet specified. Use --stylesheet." << std::endl;
    std::cout << argParser.GetHelp() << std::endl;

    return 1;
  }

  if (args.size==0 || args.dpi<=0) {
    std::cerr << "ERROR: --size and --dpi must be positive." << std::endl;

    return 1;
  }

  Backend backend;

  if (!ParseBackend(args.backendStr,backend)) {
    std::cerr << "ERROR: Invalid backend '" << args.backendStr
              << "'. Use cairo, svg or all." << std::endl;

    return 1;
  }

  bool renderCairo=(backend==Backend::Cairo || backend==Backend::All);
  bool renderSvg=(backend==Backend::Svg || backend==Backend::All);

#if !defined(HAVE_OSMSCOUT_MAP_CAIRO)
  if (renderCairo) {
    std::cerr << "ERROR: Cairo backend requested but not compiled in." << std::endl;

    return 1;
  }
#endif

#if !defined(HAVE_OSMSCOUT_MAP_SVG)
  if (renderSvg) {
    std::cerr << "ERROR: SVG backend requested but not compiled in." << std::endl;

    return 1;
  }
#endif

  if (!std::filesystem::exists(args.stylesheet)) {
    std::cerr << "ERROR: Stylesheet '" << args.stylesheet << "' does not exist." << std::endl;

    return 1;
  }

  std::string ost=ResolveOSTFile(args);

  if (ost.empty()) {
    std::cerr << "ERROR: Cannot find a type definition file for stylesheet '"
              << args.stylesheet << "'. Use --ost." << std::endl;

    return 1;
  }

  auto typeConfig=std::make_shared<osmscout::TypeConfig>();

  if (!typeConfig->LoadFromOSTFile(ost)) {
    std::cerr << "ERROR: Cannot load type definition file '" << ost << "'." << std::endl;

    return 1;
  }

  auto styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  if (!styleConfig->Load(args.stylesheet)) {
    std::cerr << "ERROR: Cannot load stylesheet '" << args.stylesheet << "'." << std::endl;

    return 1;
  }

  std::vector<std::string> names=styleConfig->GetSymbolNames();

  std::cout << "SymbolsAll: " << names.size() << " symbols in '"
            << args.stylesheet << "'" << std::endl;

  if (args.list) {
    for (const auto& name : names) {
      std::cout << name << std::endl;
    }

    return 0;
  }

  try {
    std::filesystem::create_directories(args.output);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: Cannot create output directory '" << args.output
              << "': " << e.what() << std::endl;

    return 1;
  }

  osmscout::MercatorProjection projection;

  projection.Set(osmscout::GeoCoord(50.0,14.0),
                 0.0,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 args.dpi,
                 args.size,
                 args.size);

  size_t failures=0;

#if defined(HAVE_OSMSCOUT_MAP_CAIRO)
  if (renderCairo) {
    for (const auto& name : names) {
      std::string path=(std::filesystem::path(args.output) /
                        (SanitizeFileName(name)+".png")).string();

      if (RenderSymbolCairo(projection,*styleConfig->GetSymbol(name),path,args.size)) {
        std::cout << "  OK   " << path << std::endl;
      }
      else {
        std::cerr << "  FAIL " << path << std::endl;
        ++failures;
      }
    }

    if (args.sheet) {
      std::string path=(std::filesystem::path(args.output) / "symbols.png").string();

      if (RenderSheetCairo(projection,*styleConfig,names,path,args.size)) {
        std::cout << "  OK   " << path << std::endl;
      }
      else {
        std::cerr << "  FAIL " << path << std::endl;
        ++failures;
      }
    }
  }
#endif

#if defined(HAVE_OSMSCOUT_MAP_SVG)
  if (renderSvg) {
    for (const auto& name : names) {
      std::string path=(std::filesystem::path(args.output) /
                        (SanitizeFileName(name)+".svg")).string();

      if (RenderSymbolSVG(projection,*styleConfig->GetSymbol(name),path,args.size)) {
        std::cout << "  OK   " << path << std::endl;
      }
      else {
        std::cerr << "  FAIL " << path << std::endl;
        ++failures;
      }
    }

    if (args.sheet) {
      std::string path=(std::filesystem::path(args.output) / "symbols.svg").string();

      if (RenderSheetSVG(projection,*styleConfig,names,path,args.size)) {
        std::cout << "  OK   " << path << std::endl;
      }
      else {
        std::cerr << "  FAIL " << path << std::endl;
        ++failures;
      }
    }
  }
#endif

  if (failures>0) {
    std::cerr << "FAILURE: " << failures << " render/write failure(s)." << std::endl;

    return 2;
  }

  std::cout << "Done." << std::endl;

  return 0;
}
