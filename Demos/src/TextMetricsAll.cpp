/*
  TextMetricsAll - a demo program for libosmscout
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

#include <TextMetricsAll.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/String.h>
#include <osmscoutmap/MapParameter.h>

#if defined(HAVE_OSMSCOUT_MAP_AGG)
  #include <osmscoutmapagg/MapPainterAgg.h>
  #include <agg2/agg_conv_stroke.h>
  #include <agg2/agg_path_storage.h>
  #include <agg2/agg_pixfmt_rgb.h>
  #include <agg2/agg_rasterizer_scanline_aa.h>
  #include <agg2/agg_renderer_base.h>
  #include <agg2/agg_renderer_scanline.h>
  #include <agg2/agg_rendering_buffer.h>
  #include <agg2/agg_scanline_p.h>
  #include <png.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_CAIRO)
  #if defined(__WIN32__) || defined(WIN32)
    #include <cairo.h>
  #elif defined(__APPLE__) && __APPLE__
    #include <cairo.h>
  #else
    #include <cairo/cairo.h>
  #endif
  #include <osmscoutmapcairo/MapPainterCairo.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_QT)
  #include <QApplication>
  #include <QPainter>
  #include <QPixmap>
  #include <osmscoutmapqt/MapPainterQt.h>
#endif

#if defined(HAVE_OSMSCOUT_MAP_SKIA)
  #include <osmscoutmapskia/MapPainterSkia.h>
  #include <core/SkCanvas.h>
  #include <core/SkData.h>
  #include <core/SkFont.h>
  #include <core/SkImage.h>
  #include <core/SkImageInfo.h>
  #include <core/SkPaint.h>
  #include <core/SkPixmap.h>
  #include <core/SkSurface.h>
  #include <core/SkTypeface.h>
  #include <encode/SkPngEncoder.h>
  #if defined(__linux__)
    #include <ports/SkFontMgr_fontconfig.h>
    #include <ports/SkFontScanner_FreeType.h>
  #endif
#endif

#if defined(HAVE_OSMSCOUT_MAP_SVG)
  #include <osmscoutmapsvg/MapPainterSVG.h>
#endif

namespace {

  constexpr size_t CanvasWidth = 800;
  constexpr size_t CanvasHeight = 200;
  constexpr double BaselineX = 50.0;
  constexpr double BaselineY = 100.0;

  struct Arguments
  {
    std::string text;
    std::string fontName;
    double      fontSize{1.0};
    double      fontSizeParam{2.0};
    double      dpi{96.0};
    std::string output{"textmetrics-output"};
  };

  class TextMetricsAllArgParser : public osmscout::CmdLineParser
  {
  private:
    Arguments args;

  public:
    TextMetricsAllArgParser(const std::string& appName,
                            int argc,
                            char* argv[])
    : osmscout::CmdLineParser(appName, argc, argv)
    {
      AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                args.fontName = value;
                                              }),
                "fontName",
                "Font file or family name",
                false);
      AddOption(osmscout::CmdLineDoubleOption([this](const double& value) {
                                                args.fontSize = value;
                                              }),
                "fontSize",
                "Style font size (" + std::to_string(args.fontSize) + ")",
                false);
      AddOption(osmscout::CmdLineDoubleOption([this](const double& value) {
                                                args.fontSizeParam = value;
                                              }),
                "fontSizeParam",
                "Map parameter font size (" + std::to_string(args.fontSizeParam) + ")",
                false);
      AddOption(osmscout::CmdLineDoubleOption([this](const double& value) {
                                                args.dpi = value;
                                              }),
                "dpi",
                "Rendering DPI (" + std::to_string(args.dpi) + ")",
                false);
      AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                args.output = value;
                                              }),
                "output",
                "Output directory (" + args.output + ")",
                false);
      AddPositional(osmscout::CmdLineStringOption([this](const std::string& value) {
                                                    args.text = value;
                                                  }),
                    "text",
                    "Text to measure");
    }

    Arguments GetArguments() const
    {
      return args;
    }
  };

  /**
   * Extract a font family name from a font file path (basename without
   * extension). Backends that resolve fonts by family name (Cairo/Pango,
   * Qt, SVG) cannot load a file path directly.
   */
  std::string FontFamilyName(const std::string& fontName)
  {
    std::string base = std::filesystem::path(fontName).filename().string();
    size_t      dot = base.find_last_of('.');

    if (dot != std::string::npos) {
      base = base.substr(0, dot);
    }

    return base;
  }

  /**
   * Print label dimensions, per-glyph boxes and the difference to the
   * FreeType reference for one backend.
   */
  void PrintBackendMetrics(const std::string& name,
                           const osmscout::TextMetrics& metrics,
                           const TextMetricsAll::ReferenceMetrics& reference)
  {
    std::cout << "  " << name << ": label width=" << metrics.width
              << " height=" << metrics.height << std::endl;

    for (size_t i = 0; i < metrics.glyphs.size(); ++i) {
      const auto & glyph = metrics.glyphs[i];

      std::cout << "    glyph " << i
                << ": pos=(" << glyph.position.GetX() << ", " << glyph.position.GetY() << ")"
                << " box=(" << glyph.box.x << ", " << glyph.box.y << ", "
                << glyph.box.width << ", " << glyph.box.height << ")";
      if (i < reference.glyphs.size()) {
        const auto & ref = reference.glyphs[i];

        std::cout << " diff=(" << (glyph.box.x - ref.x) << ", " << (glyph.box.y - ref.y)
                  << ", " << (glyph.box.width - ref.width) << ", " << (glyph.box.height - ref.height) << ")";
      }
      else {
        std::cout << " diff=(no reference)";
      }
      std::cout << std::endl;
    }

    if (metrics.glyphs.size() != reference.glyphs.size()) {
      std::cout << "    WARNING: glyph count " << metrics.glyphs.size()
                << " differs from reference " << reference.glyphs.size() << std::endl;
    }
  }

#if defined(HAVE_OSMSCOUT_MAP_AGG)

  /**
   * Write a raw RGB buffer to a PNG file (libpng).
   */
  bool WriteRGBToPNG(const std::string& path,
                     size_t width,
                     size_t height,
                     const unsigned char* rgb)
  {
    FILE * fp = fopen(path.c_str(), "wb");

    if (!fp) {
      std::cerr << "ERROR: Cannot open " << path << " for writing" << std::endl;

      return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                              nullptr,
                                              nullptr,
                                              nullptr);

    if (!png) {
      fclose(fp);

      return false;
    }

    png_infop info = png_create_info_struct(png);

    if (!info) {
      png_destroy_write_struct(&png, nullptr);
      fclose(fp);

      return false;
    }

    if (setjmp(png_jmpbuf(png))) {
      png_destroy_write_struct(&png, &info);
      fclose(fp);

      return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png,
                 info,
                 (png_uint_32)width,
                 (png_uint_32)height,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    size_t                 rowStride = width * 3;
    std::vector<png_bytep> rowPointers(height);

    for (size_t y = 0; y < height; ++y) {
      rowPointers[y] = const_cast<png_bytep>(rgb + y * rowStride);
    }

    png_write_image(png, rowPointers.data());
    png_write_end(png, info);

    png_destroy_write_struct(&png, &info);
    fclose(fp);

    return true;
  }

#endif
} // namespace

int main(int argc, char* argv[])
{
  TextMetricsAllArgParser      argParser("TextMetricsAll", argc, argv);

  osmscout::CmdLineParseResult argResult = argParser.Parse();

  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;

    return 1;
  }

  Arguments args = argParser.GetArguments();

  if (args.text.empty()) {
    std::cerr << "ERROR: No text specified." << std::endl;
    std::cout << argParser.GetHelp() << std::endl;

    return 1;
  }

  if (args.fontName.empty()) {
    std::cerr << "ERROR: No font specified. Use --fontName." << std::endl;

    return 1;
  }

  if (args.fontSize <= 0 || args.fontSizeParam <= 0 || args.dpi <= 0) {
    std::cerr << "ERROR: fontSize, fontSizeParam and dpi must be positive." << std::endl;

    return 1;
  }

  try {
    std::filesystem::create_directories(args.output);
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Cannot create output directory '" << args.output
              << "': " << e.what() << std::endl;

    return 1;
  }

  // Pixel size shared by all backends and the reference
  double px = TextMetricsAll::ReferencePixelSize(args.fontSize, args.fontSizeParam, args.dpi);

  // FreeType reference measurement (only when FreeType is compiled in)
  TextMetricsAll::ReferenceMetrics reference;

#if defined(HAVE_LIB_FREETYPE)
  std::string error;

  if (!TextMetricsAll::MeasureReference(args.fontName,
                                        args.text,
                                        args.fontSize,
                                        args.fontSizeParam,
                                        args.dpi,
                                        reference,
                                        error)) {
    std::cerr << "ERROR: Reference measurement failed: " << error << std::endl;

    return 1;
  }
#endif

  std::cout << "TextMetricsAll: measuring \"" << args.text << "\"" << std::endl
            << "  font: " << args.fontName << std::endl
            << "  pixel size: " << px << " px (fontSize=" << args.fontSize
            << " fontSizeParam=" << args.fontSizeParam << " dpi=" << args.dpi << ")" << std::endl
            << "  output directory: " << args.output << std::endl;

#if defined(HAVE_LIB_FREETYPE)
  std::cout << "  Reference (FreeType): label width=" << reference.width
            << " height=" << reference.height << std::endl;
  for (size_t i = 0; i < reference.glyphs.size(); ++i) {
    const auto & glyph = reference.glyphs[i];

    std::cout << "    glyph " << i
              << ": box=(" << glyph.x << ", " << glyph.y << ", "
              << glyph.width << ", " << glyph.height << ")"
              << " advance=" << glyph.advance << std::endl;
  }
#else
  std::cout << "  Reference (FreeType): unavailable (not compiled in)" << std::endl;

#endif

  osmscout::MercatorProjection projection;

  projection.Set(osmscout::GeoCoord(50.0, 14.0),
                 0.0,
                 osmscout::Magnification(osmscout::Magnification::magClose),
                 args.dpi,
                 CanvasWidth,
                 CanvasHeight);

  osmscout::MapParameter parameter;

  parameter.SetFontName(args.fontName);
  parameter.SetFontSize(args.fontSizeParam);

  // ================================================================
  // AGG backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_AGG)
  {
    std::cout << "AGG backend..." << std::endl;

    auto * buffer = new unsigned char[CanvasWidth * CanvasHeight * 3];

    memset(buffer, 255, CanvasWidth * CanvasHeight * 3);

    agg::rendering_buffer rbuf(buffer, (unsigned int)CanvasWidth,
                               (unsigned int)CanvasHeight,
                               (int)(CanvasWidth * 3));
    agg::pixfmt_rgb24                                                      pf(rbuf);
    agg::renderer_base<agg::pixfmt_rgb24>                                  rendererBase(pf);
    agg::rasterizer_scanline_aa<>                                          rasterizer;
    agg::scanline_p8                                                       scanline;
    agg::renderer_scanline_aa_solid<agg::renderer_base<agg::pixfmt_rgb24>> rendererAA(rendererBase);

    osmscout::MapPainterAgg                                                painter;
    auto                                                                   metrics = painter.MeasureText(projection,
                                                                                                         parameter,
                                                                                                         args.text,
                                                                                                         args.fontSize);

    PrintBackendMetrics("AGG", metrics, reference);

    // Draw text natively with the AGG FreeType font engine
    agg::font_engine_freetype_int32                          fontEngine;
    agg::font_cache_manager<agg::font_engine_freetype_int32> fontCacheManager(fontEngine);

    if (fontEngine.load_font(args.fontName.c_str(), 0, agg::glyph_ren_native_gray8)) {
      fontEngine.width(px);
      fontEngine.height(px);
      fontEngine.hinting(true);
      fontEngine.flip_y(true);

      rendererAA.color(agg::rgba(0, 0, 0, 1));

      double x = BaselineX;
      double y = BaselineY;

      for (wchar_t ch : osmscout::UTF8StringToWString(args.text)) {
        const agg::glyph_cache * glyph = fontCacheManager.glyph(ch);

        if (glyph != nullptr) {
          fontCacheManager.add_kerning(&x, &y);
          fontCacheManager.init_embedded_adaptors(glyph, x, y);
          agg::render_scanlines(fontCacheManager.gray8_adaptor(),
                                fontCacheManager.gray8_scanline(),
                                rendererAA);
          x += glyph->advance_x;
          y += glyph->advance_y;
        }
      }
    }
    else {
      std::cerr << "  WARNING: AGG cannot load font '" << args.fontName << "'" << std::endl;
    }

    // Overlay per-glyph bounding boxes
    rendererAA.color(agg::rgba(1, 0, 0, 1));
    for (const auto& glyph : metrics.glyphs) {
      double            bx = BaselineX + glyph.position.GetX() + glyph.box.x;
      double            by = BaselineY + glyph.position.GetY() + glyph.box.y;

      agg::path_storage box;

      box.move_to(bx, by);
      box.line_to(bx + glyph.box.width, by);
      box.line_to(bx + glyph.box.width, by + glyph.box.height);
      box.line_to(bx, by + glyph.box.height);
      box.close_polygon();

      agg::conv_stroke<agg::path_storage> stroke(box);

      stroke.width(1.0);

      rasterizer.reset();
      rasterizer.add_path(stroke);
      agg::render_scanlines(rasterizer, scanline, rendererAA);
    }

    std::string path = args.output + "/TextMetricsAgg.png";

    if (WriteRGBToPNG(path, CanvasWidth, CanvasHeight, buffer)) {
      std::cout << "  OK: " << path << std::endl;
    }

    delete[] buffer;
  }
#else
  std::cout << "AGG backend: unavailable (not compiled in)" << std::endl;
#endif

  // ================================================================
  // Cairo backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_CAIRO)
  {
    std::cout << "Cairo backend..." << std::endl;

    cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                           (int)CanvasWidth,
                                                           (int)CanvasHeight);

    if (surface == nullptr) {
      std::cerr << "  FAIL: Cairo surface creation" << std::endl;
    }
    else {
      cairo_t * cr = cairo_create(surface);

      if (cr == nullptr) {
        std::cerr << "  FAIL: Cairo context creation" << std::endl;
      }
      else {
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);

        osmscout::MapPainterCairo painter;

        // DrawMap sets the internal cairo context used by Layout()/MeasureText()
        painter.DrawMap(projection, parameter, {}, cr);

        auto metrics = painter.MeasureText(projection, parameter, args.text, args.fontSize);

        PrintBackendMetrics("Cairo", metrics, reference);

        // Draw text natively
        std::string family = FontFamilyName(args.fontName);

        cairo_select_font_face(cr,
                               family.c_str(),
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, px);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, BaselineX, BaselineY);
        cairo_show_text(cr, args.text.c_str());

        // Overlay per-glyph bounding boxes
        cairo_set_source_rgb(cr, 1, 0, 0);
        cairo_set_line_width(cr, 1.0);
        for (const auto& glyph : metrics.glyphs) {
          double bx = BaselineX + glyph.position.GetX() + glyph.box.x;
          double by = BaselineY + glyph.position.GetY() + glyph.box.y;

          cairo_rectangle(cr, bx, by, glyph.box.width, glyph.box.height);
          cairo_stroke(cr);
        }

        std::string path = args.output + "/TextMetricsCairo.png";

        if (cairo_surface_write_to_png(surface, path.c_str()) == CAIRO_STATUS_SUCCESS) {
          std::cout << "  OK: " << path << std::endl;
        }
        else {
          std::cerr << "  FAIL: Cairo PNG write" << std::endl;
        }

        cairo_destroy(cr);
      }
      cairo_surface_destroy(surface);
    }
  }
#else
  std::cout << "Cairo backend: unavailable (not compiled in)" << std::endl;
#endif

  // ================================================================
  // Qt backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_QT)
  {
    std::cout << "Qt backend..." << std::endl;

    int          qtArgc = 1;
    char         qtArg0[] = "TextMetricsAll";
    char         * qtArgv[1] = {qtArg0};
    QApplication qtApp(qtArgc, qtArgv, true);

    QPixmap      pixmap((int)CanvasWidth, (int)CanvasHeight);

    pixmap.fill(Qt::white);

    QPainter               qp(&pixmap);

    osmscout::MapPainterQt painter;

    // DrawMap sets the internal QPainter used by Layout()/MeasureText()
    painter.DrawMap(projection, parameter, {}, &qp);

    auto metrics = painter.MeasureText(projection, parameter, args.text, args.fontSize);

    PrintBackendMetrics("Qt", metrics, reference);

    // Draw text natively (drawText(x, y, ...) uses the baseline)
    QFont font(QString::fromStdString(FontFamilyName(args.fontName)));

    font.setPixelSize((int)std::lround(px));
    qp.setFont(font);
    qp.setPen(Qt::black);
    qp.drawText(QPointF(BaselineX, BaselineY), QString::fromStdString(args.text));

    // Overlay per-glyph bounding boxes
    qp.setPen(QPen(Qt::red, 1.0));
    for (const auto& glyph : metrics.glyphs) {
      double bx = BaselineX + glyph.position.GetX() + glyph.box.x;
      double by = BaselineY + glyph.position.GetY() + glyph.box.y;

      qp.drawRect(QRectF(bx, by, glyph.box.width, glyph.box.height));
    }
    qp.end();

    std::string path = args.output + "/TextMetricsQt.png";

    if (pixmap.save(QString::fromStdString(path), "PNG", -1)) {
      std::cout << "  OK: " << path << std::endl;
    }
    else {
      std::cerr << "  FAIL: Qt PNG write" << std::endl;
    }
  }
#else
  std::cout << "Qt backend: unavailable (not compiled in)" << std::endl;
#endif

  // ================================================================
  // Skia backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_SKIA)
  {
    std::cout << "Skia backend..." << std::endl;

    SkImageInfo      info = SkImageInfo::MakeN32Premul((int)CanvasWidth, (int)CanvasHeight);
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);

    if (!surface) {
      std::cerr << "  FAIL: Cannot create Skia surface" << std::endl;
    }
    else {
      SkCanvas * canvas = surface->getCanvas();

      canvas->clear(SK_ColorWHITE);

      osmscout::MapPainterSkia painter;
      auto                     metrics = painter.MeasureText(projection, parameter, args.text, args.fontSize);

      PrintBackendMetrics("Skia", metrics, reference);

      // Draw text natively (drawString uses the baseline)
      sk_sp<SkTypeface> typeface;

#if defined(__linux__)
      sk_sp<SkFontMgr> fontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());

      if (fontMgr) {
        typeface = fontMgr->legacyMakeTypeface(args.fontName.c_str(), SkFontStyle::Normal());
      }
#endif
      if (!typeface) {
        typeface = SkTypeface::MakeEmpty();
      }

      SkFont font(typeface, (SkScalar)px);

      font.setSubpixel(true);

      SkPaint textPaint;

      textPaint.setAntiAlias(true);
      textPaint.setColor(SK_ColorBLACK);
      canvas->drawString(args.text.c_str(),
                         (SkScalar)BaselineX,
                         (SkScalar)BaselineY,
                         font,
                         textPaint);

      // Overlay per-glyph bounding boxes
      SkPaint boxPaint;

      boxPaint.setColor(SK_ColorRED);
      boxPaint.setStyle(SkPaint::kStroke_Style);
      boxPaint.setStrokeWidth(1.0f);
      for (const auto& glyph : metrics.glyphs) {
        double bx = BaselineX + glyph.position.GetX() + glyph.box.x;
        double by = BaselineY + glyph.position.GetY() + glyph.box.y;

        canvas->drawRect(SkRect::MakeXYWH((SkScalar)bx,
                                          (SkScalar)by,
                                          (SkScalar)glyph.box.width,
                                          (SkScalar)glyph.box.height),
                         boxPaint);
      }

      sk_sp<SkImage> image = surface->makeImageSnapshot();
      SkPixmap       pixmap;
      std::string    path = args.output + "/TextMetricsSkia.png";

      if (image && image->peekPixels(&pixmap)) {
        sk_sp<SkData> png = SkPngEncoder::Encode(pixmap, SkPngEncoder::Options{});

        if (png) {
          FILE * fp = fopen(path.c_str(), "wb");

          if (fp) {
            fwrite(png->data(), 1, png->size(), fp);
            fclose(fp);
            std::cout << "  OK: " << path << std::endl;
          }
          else {
            std::cerr << "  FAIL: Cannot open " << path << " for writing" << std::endl;
          }
        }
        else {
          std::cerr << "  FAIL: Skia PNG encode" << std::endl;
        }
      }
      else {
        std::cerr << "  FAIL: Skia pixel read" << std::endl;
      }
    }
  }
#else
  std::cout << "Skia backend: unavailable (not compiled in)" << std::endl;
#endif

  // ================================================================
  // SVG backend
  // ================================================================
#if defined(HAVE_OSMSCOUT_MAP_SVG)
  {
    std::cout << "SVG backend..." << std::endl;

    osmscout::MapPainterSVG painter;
    auto                    metrics = painter.MeasureText(projection, parameter, args.text, args.fontSize);

    PrintBackendMetrics("SVG", metrics, reference);

    std::string   path = args.output + "/TextMetricsSVG.svg";
    std::ofstream stream(path.c_str(),
                         std::ios_base::binary |
                         std::ios_base::trunc |
                         std::ios_base::out);

    if (!stream) {
      std::cerr << "  FAIL: Cannot open " << path << " for writing" << std::endl;
    }
    else {
      stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
      stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << CanvasWidth
             << "\" height=\"" << CanvasHeight << "\">" << std::endl;
      stream << "  <rect width=\"100%\" height=\"100%\" fill=\"white\"/>" << std::endl;
      // SVG <text> y is the baseline
      stream << "  <text x=\"" << BaselineX << "\" y=\"" << BaselineY
             << "\" font-family=\"" << FontFamilyName(args.fontName) << "\" font-size=\"" << px
             << "\" fill=\"black\">" << args.text << "</text>" << std::endl;
      for (const auto& glyph : metrics.glyphs) {
        double bx = BaselineX + glyph.position.GetX() + glyph.box.x;
        double by = BaselineY + glyph.position.GetY() + glyph.box.y;

        stream << "  <rect x=\"" << bx << "\" y=\"" << by
               << "\" width=\"" << glyph.box.width << "\" height=\"" << glyph.box.height
               << "\" fill=\"none\" stroke=\"red\" stroke-width=\"1\"/>" << std::endl;
      }
      stream << "</svg>" << std::endl;
      stream.close();
      std::cout << "  OK: " << path << std::endl;
    }
  }
#else
  std::cout << "SVG backend: unavailable (not compiled in)" << std::endl;
#endif

  return 0;
}
