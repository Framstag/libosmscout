/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscoutmapcairo/MapPainterCairo.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <list>

#include <osmscoutmapcairo/LoaderPNG.h>
#include <osmscoutmapcairo/SymbolRendererCairo.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>
#include <osmscout/util/String.h>

namespace osmscout {

  /* Returns Euclidean distance between two points */
  static double CalculatePointDistance(cairo_path_data_t *a, cairo_path_data_t *b)
  {
    double dx = b->point.x - a->point.x;
    double dy = b->point.y - a->point.y;

    return sqrt(dx * dx + dy * dy);
  }

  /**
   * Calculate an array of double for the path, that contains the length of each path segment
   */
  static double *CalculatePathSegmentLengths(cairo_path_t *path)
  {
    cairo_path_data_t *startPoint = nullptr;
    cairo_path_data_t *currentPoint = nullptr;
    double *parametrization;

    parametrization = new double[path->num_data];

    for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
      cairo_path_data_t *data = &path->data[i];

      parametrization[i] = 0.0;

      switch (data->header.type) {
        case CAIRO_PATH_MOVE_TO:
          startPoint = &data[1];
          currentPoint = &data[1];
          break;
        case CAIRO_PATH_CLOSE_PATH:
          // From current point back to the start */
          parametrization[i] = CalculatePointDistance(currentPoint, startPoint);
          currentPoint = startPoint;
          break;
        case CAIRO_PATH_LINE_TO:
          parametrization[i] = CalculatePointDistance(currentPoint, &data[1]);
          currentPoint = &data[1];
          break;
        case CAIRO_PATH_CURVE_TO:
          assert(false);
          break;
        default:
          assert(false);
          break;
      }
    }

    return parametrization;
  }

  /**
   *  Project a point X,Y onto a parameterized path. The final point is
   * where you get if you walk on the path forward from the beginning for X
   * units, then stop there and walk another Y units perpendicular to the
   * path at that point. In more detail:
   *
   * There's two pieces of math involved:
   *
   * - The parametric form of the Line equation
   * http://en.wikipedia.org/wiki/Line
   *
   * - The Gradient (aka multi-dimensional derivative) of the above
   * http://en.wikipedia.org/wiki/Gradient
   *
   * The parametric forms are used to answer the question of "where will I be
   * if I walk a distance of X on this path". The Gradient is used to answer
   * the question of "where will I be if then I stop, rotate left for 90
   * degrees and walk straight for a distance of Y".
   */
  static void PathPointTransformer(double &x,
                                   double &y,
                                   cairo_path_t *path,
                                   const double *pathSegmentLengths)
  {
    int i;
    double the_y = y;
    double the_x = x;
    cairo_path_data_t *data;
    cairo_path_data_t *startPoint = nullptr;
    cairo_path_data_t *currentPoint = nullptr;

    // Find the segment on the line that is "x" away from the start
    for (i = 0;
         i + path->data[i].header.length < path->num_data &&
         (the_x > pathSegmentLengths[i] ||
          path->data[i].header.type == CAIRO_PATH_MOVE_TO);
         i += path->data[i].header.length) {
      data = &path->data[i];

      the_x -= pathSegmentLengths[i];

      switch (data->header.type) {
        case CAIRO_PATH_MOVE_TO:
          currentPoint = &data[1];
          startPoint = &data[1];
          break;
        case CAIRO_PATH_LINE_TO:
          currentPoint = &data[1];
          break;
        case CAIRO_PATH_CURVE_TO:
          assert(false);
          return;
        case CAIRO_PATH_CLOSE_PATH:
          break;
        default:
          assert(false);
          return;
      }
    }

    data = &path->data[i];

    switch (data->header.type) {

      case CAIRO_PATH_MOVE_TO:
        break;
      case CAIRO_PATH_CLOSE_PATH: {
        // Relative offset in the current segment ([0..1])
        double ratio = the_x / pathSegmentLengths[i];
        // Line polynomial
        x = currentPoint->point.x * (1 - ratio) + startPoint->point.x * ratio;
        y = currentPoint->point.y * (1 - ratio) + startPoint->point.y * ratio;

        // Line gradient
        double dx = -(currentPoint->point.x - startPoint->point.x);
        double dy = -(currentPoint->point.y - startPoint->point.y);

        // optimization for: ratio = the_y / sqrt (dx * dx + dy * dy)
        ratio = the_y / pathSegmentLengths[i];
        x += -dy * ratio;
        y += dx * ratio;
      }
        break;
      case CAIRO_PATH_LINE_TO: {
        // Relative offset in the current segment ([0..1])
        double ratio = the_x / pathSegmentLengths[i];
        // Line polynomial
        x = currentPoint->point.x * (1 - ratio) + data[1].point.x * ratio;
        y = currentPoint->point.y * (1 - ratio) + data[1].point.y * ratio;

        // Line gradient
        double dx = -(currentPoint->point.x - data[1].point.x);
        double dy = -(currentPoint->point.y - data[1].point.y);

        // optimization for: ratio = the_y / sqrt (dx * dx + dy * dy)
        ratio = the_y / pathSegmentLengths[i];
        x += -dy * ratio;
        y += dx * ratio;
      }
        break;
      case CAIRO_PATH_CURVE_TO:
        assert(false);
        break;
      default:
        assert(false);
        break;
    }
  }

  /**
   * Project a path using a function. Each point of the path (including
   * Bezier control points) is passed to the function for transformation.
   */
  static void TransformPathOntoPath(cairo_path_t *srcPath,
                                    cairo_path_t *dstPath,
                                    double *pathSegmentLengths,
                                    double xOffset,
                                    double yOffset)
  {
    for (int i = 0; i < srcPath->num_data; i += srcPath->data[i].header.length) {
      cairo_path_data_t *data = &srcPath->data[i];

      switch (data->header.type) {
        case CAIRO_PATH_CURVE_TO:
          data[1].point.x += xOffset;
          data[1].point.y += yOffset;
          data[2].point.x += xOffset;
          data[2].point.y += yOffset;
          data[3].point.x += xOffset;
          data[3].point.y += yOffset;
          PathPointTransformer(data[1].point.x, data[1].point.y, dstPath, pathSegmentLengths);
          PathPointTransformer(data[2].point.x, data[2].point.y, dstPath, pathSegmentLengths);
          PathPointTransformer(data[3].point.x, data[3].point.y, dstPath, pathSegmentLengths);
          break;
        case CAIRO_PATH_MOVE_TO:
          data[1].point.x += xOffset;
          data[1].point.y += yOffset;
          PathPointTransformer(data[1].point.x, data[1].point.y, dstPath, pathSegmentLengths);
          break;
        case CAIRO_PATH_LINE_TO:
          data[1].point.x += xOffset;
          data[1].point.y += yOffset;
          PathPointTransformer(data[1].point.x, data[1].point.y, dstPath, pathSegmentLengths);
          break;
        case CAIRO_PATH_CLOSE_PATH:
          break;
        default:
          assert(false);
          break;
      }
    }
  }


  /* Projects the current path of cr onto the provided path. */
  static void MapPathOnPath(cairo_t *draw,
                            cairo_path_t *srcPath,
                            cairo_path_t *dstPath,
                            double xOffset,
                            double yOffset)
  {
    double *segmentLengths = CalculatePathSegmentLengths(dstPath);

    // Center on path
    TransformPathOntoPath(srcPath,
                          dstPath,
                          segmentLengths,
                          xOffset,
                          yOffset);

    cairo_new_path(draw);
    cairo_append_path(draw, srcPath);

    delete[] segmentLengths;
  }

  MapPainterCairo::MapPainterCairo(const StyleConfigRef &styleConfig)
      : MapPainter(styleConfig),
        labelLayouter(this)
  {
    // no code
  }

  MapPainterCairo::~MapPainterCairo()
  {
    for (const auto &image : images) {
      if (image != nullptr) {
        cairo_surface_destroy(image);
      }
    }

    for (const auto &pattern : patterns) {
      if (pattern != nullptr) {
        cairo_pattern_destroy(pattern);
      }
    }

    for (const auto &image : patternImages) {
      if (image != nullptr) {
        cairo_surface_destroy(image);
      }
    }

    for (const auto &entry : fonts) {
      if (entry.second != nullptr) {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
        pango_font_description_free(entry.second);
#else
        cairo_scaled_font_destroy(entry.second);
#endif
      }
    }
  }

  MapPainterCairo::CairoFont MapPainterCairo::GetFont(const Projection &projection,
                                                      const MapParameter &parameter,
                                                      double fontSize)
  {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    FontMap::const_iterator f;

    fontSize = fontSize * projection.ConvertWidthToPixel(parameter.GetFontSize());

    f = fonts.find(fontSize);

    if (f != fonts.end()) {
      return f->second;
    }

    PangoFontDescription *font = pango_font_description_new();

    pango_font_description_set_family(font, parameter.GetFontName().c_str());
    pango_font_description_set_absolute_size(font, fontSize * PANGO_SCALE);

    return fonts.insert(std::make_pair(fontSize, font)).first->second;
#else
    FontMap::const_iterator f;

    fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    cairo_font_face_t    *fontFace;
    cairo_matrix_t       scaleMatrix;
    cairo_matrix_t       transformMatrix;
    cairo_font_options_t *options;
    cairo_scaled_font_t  *scaledFont;

    fontFace=cairo_toy_font_face_create(parameter.GetFontName().c_str(),
                                        CAIRO_FONT_SLANT_NORMAL,
                                        CAIRO_FONT_WEIGHT_NORMAL);

    cairo_matrix_init_scale(&scaleMatrix,fontSize,fontSize);
    cairo_matrix_init_identity(&transformMatrix);

    options=cairo_font_options_create();
    cairo_font_options_set_hint_style (options,CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics (options,CAIRO_HINT_METRICS_OFF);

    scaledFont=cairo_scaled_font_create(fontFace,
                                        &scaleMatrix,
                                        &transformMatrix,
                                        options);

    cairo_font_options_destroy(options);
    cairo_font_face_destroy(fontFace);

    return fonts.insert(std::make_pair(fontSize,scaledFont)).first->second;
#endif
  }

  void MapPainterCairo::SetLineAttributes(const Color &color,
                                          double width,
                                          const std::vector<double> &dash)
  {
    assert(dash.size() <= 10);

    cairo_set_source_rgba(draw,
                          color.GetR(),
                          color.GetG(),
                          color.GetB(),
                          color.GetA());

    cairo_set_line_width(draw, width);

    if (dash.empty()) {
      cairo_set_dash(draw, nullptr, 0, 0);
    }
    else {
      std::array<double,10> dashArray;

      for (size_t i = 0; i < dash.size(); i++) {
        dashArray[i] = dash[i] * width;
      }
      cairo_set_dash(draw, dashArray.data(), static_cast<int>(dash.size()), 0.0);
    }
  }

  void MapPainterCairo::DrawFillStyle(const Projection &projection,
                                      const MapParameter &parameter,
                                      const FillStyleRef &fill,
                                      const BorderStyleRef &border)
  {
    bool hasFill = false;
    bool hasBorder = false;

    if (fill) {
      if (fill->HasPattern() &&
          projection.GetMagnification() >= fill->GetPatternMinMag() &&
          HasPattern(parameter, *fill)) {
        size_t idx = fill->GetPatternId() - 1;

        assert(idx < patterns.size());
        assert(patterns[idx] != nullptr);

        cairo_set_source(draw, patterns[idx]);
        hasFill = true;
      } else if (fill->GetFillColor().IsVisible()) {
        Color color = fill->GetFillColor();
        cairo_set_source_rgba(draw,
                              color.GetR(),
                              color.GetG(),
                              color.GetB(),
                              color.GetA());
        hasFill = true;
      }
    }

    if (border) {
      hasBorder = border->GetWidth() > 0 &&
                  border->GetColor().IsVisible() &&
                  border->GetWidth() >= minimumLineWidth;
    }

    if (hasFill && hasBorder) {
      cairo_fill_preserve(draw);
    } else if (hasFill) {
      cairo_fill(draw);
    }

    if (hasBorder) {
      double borderWidth = projection.ConvertWidthToPixel(border->GetWidth());

      if (borderWidth >= parameter.GetLineMinWidthPixel()) {
        SetLineAttributes(border->GetColor(),
                          borderWidth,
                          border->GetDash());

        cairo_set_line_cap(draw, CAIRO_LINE_CAP_BUTT);

        cairo_stroke(draw);
      }
    }
  }

  bool MapPainterCairo::HasIcon(const StyleConfig & /*styleConfig*/,
                                const Projection& projection,
                                const MapParameter &parameter,
                                IconStyle &style)
  {
    // Already loaded with error
    if (style.GetIconId() == 0) {
      return false;
    }

    size_t idx = style.GetIconId() - 1;

    // there is possible that exists multiple IconStyle instances with same iconId (point and area icon with same icon name)
    // setup dimensions for all of them
    if (parameter.GetIconMode()==MapParameter::IconMode::Scalable ||
        parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap){

      style.SetWidth(std::round(projection.ConvertWidthToPixel(parameter.GetIconSize())));
      style.SetHeight(style.GetWidth());
    }else{
      style.SetWidth(std::round(parameter.GetIconPixelSize()));
      style.SetHeight(style.GetWidth());
    }

    // Already cached?
    if (idx < images.size() &&
        images[idx] != nullptr) {

      if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap){
        style.SetWidth(cairo_image_surface_get_width(images[idx]));
        style.SetHeight(cairo_image_surface_get_height(images[idx]));
      }
      return true;
    }

    for (std::list<std::string>::const_iterator path = parameter.GetIconPaths().begin();
         path != parameter.GetIconPaths().end();
         ++path) {

      // TODO: add support for reading svg images (using librsvg?)
      std::string filename = *path + style.GetIconName() + ".png";

      cairo_surface_t *image = osmscout::LoadPNG(filename);

      if (image != nullptr) {
        if (idx >= images.size()) {
          images.resize(idx + 1, nullptr);
        }

        if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap){
          style.SetWidth(cairo_image_surface_get_width(image));
          style.SetHeight(cairo_image_surface_get_height(image));
        }

        images[idx] = image;

        std::cout << "Loaded image '" << filename << "'" << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading image '" << style.GetIconName() << "'" << std::endl;
    style.SetIconId(0);

    return false;
  }

  bool MapPainterCairo::HasPattern(const MapParameter &parameter,
                                   const FillStyle &style)
  {
    assert(style.HasPattern());

    // Already loaded with error
    if (style.GetPatternId() == 0) {
      return false;
    }

    size_t idx = style.GetPatternId() - 1;

    // Already cached?
    if (idx < patterns.size() &&
        patterns[idx] != nullptr) {
      return true;
    }

    for (const auto & path : parameter.GetPatternPaths()) {
      std::string filename = path + style.GetPatternName() + ".png";

      cairo_surface_t *image = osmscout::LoadPNG(filename);

      if (image != nullptr) {
        if (idx >= patternImages.size()) {
          patternImages.resize(idx + 1, nullptr);
        }

        patternImages[idx] = image;

        if (idx >= patterns.size()) {
          patterns.resize(idx + 1, nullptr);
        }

        patterns[idx] = cairo_pattern_create_for_surface(patternImages[idx]);
        cairo_pattern_set_extend(patterns[idx], CAIRO_EXTEND_REPEAT);

        cairo_matrix_t matrix;

        cairo_matrix_init_scale(&matrix, 1, 1);
        cairo_pattern_set_matrix(patterns[idx], &matrix);

        std::cout << "Loaded pattern image '" << filename << "'" << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading pattern image '" << style.GetPatternName() << "'" << std::endl;
    style.SetPatternId(0);

    return false;
  }

  double MapPainterCairo::GetFontHeight(const Projection &projection,
                                        const MapParameter &parameter,
                                        double fontSize)
  {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    CairoFont font;

    font = GetFont(projection,
                   parameter,
                   fontSize);

    return pango_font_description_get_size(font) / PANGO_SCALE;
#else
    CairoFont            font;
    cairo_font_extents_t fontExtents;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    cairo_scaled_font_extents(font,&fontExtents);

    return fontExtents.height;
#endif
  }

  void MapPainterCairo::DrawContourSymbol(const Projection &projection,
                                          const MapParameter &/*parameter*/,
                                          const Symbol &symbol,
                                          const ContourSymbolData& data)
  {
    double lineLength = data.coordRange.GetLength();

    cairo_save(draw);
    cairo_new_path(draw);

    cairo_move_to(draw,
                  data.coordRange.GetFirst().GetX(),
                  data.coordRange.GetFirst().GetY());

    for (size_t j = data.coordRange.GetStart()+1; j <= data.coordRange.GetEnd(); j++) {
      Vertex2D currentCoord=data.coordRange.Get(j);
      cairo_line_to(draw,
                    currentCoord.GetX(),
                    currentCoord.GetY());
    }

    cairo_path_t *path = cairo_copy_path_flat(draw);

    ScreenBox boundingBox=symbol.GetBoundingBox(projection);
    double    width=boundingBox.GetWidth();
    double    height=boundingBox.GetHeight();

    double currentPos = data.symbolOffset;

    while (currentPos + width < lineLength) {
      SymbolRendererCairo renderer(draw);

      cairo_path_t* patternPath;

      renderer.Render(projection,
                      symbol,
                      Vertex2D(currentPos+width/2.0, 0.0),
                      [&patternPath,this,&path,&height]() {
                        patternPath=cairo_copy_path_flat(draw);
                        MapPathOnPath(draw,
                                      patternPath,
                                      path,
                                      0,
                                      height/2);
                      },
                      [&patternPath]() {
                        cairo_path_destroy(patternPath);
                      },
      data.symbolScale);

      currentPos+=width+data.symbolSpace;
    }

    cairo_path_destroy(path);

    cairo_restore(draw);
  }

  void MapPainterCairo::BeforeDrawing(const StyleConfig& /*styleConfig*/,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& /*data*/)
  {
    DoubleRectangle viewport;
    double x2, y2;
    cairo_clip_extents(draw, &viewport.x, &viewport.y, &x2, &y2);
    viewport.width = x2 - viewport.x;
    viewport.height = y2 - viewport.y;

    labelLayouter.SetViewport(viewport);
    labelLayouter.SetLayoutOverlap(projection.ConvertWidthToPixel(parameter.GetLabelLayouterOverlap()));
  }

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)

  template<>
  std::vector<Glyph<MapPainterCairo::CairoNativeGlyph>> MapPainterCairo::CairoLabel::ToGlyphs() const
  {
    PangoRectangle extends;

    pango_layout_get_pixel_extents(label.get(),
                                   nullptr,
                                   &extends);

    // label is centered - we have to move its left horizontal offset
    double horizontalOffset = extends.x * -1.0;
    std::vector<Glyph<MapPainterCairo::CairoNativeGlyph>> result;

    if constexpr (debugLabelLayouter) {
      std::cout << " = getting glyphs for label: " << text << std::endl;
    }

    for (PangoLayoutIter *iter = pango_layout_get_iter(label.get());
         iter != nullptr;){
      PangoLayoutRun *run = pango_layout_iter_get_run_readonly(iter);
      if (run == nullptr) {
        pango_layout_iter_free(iter);
        break; // nullptr signalise end of line, we don't expect more lines in contour label
      }

      std::shared_ptr<PangoFont> font = std::shared_ptr<PangoFont>(run->item->analysis.font,
                                                                   g_object_unref);
      g_object_ref(font.get());

      if constexpr (debugLabelLayouter) {
        std::cout << "   run with " << run->glyphs->num_glyphs << " glyphs (font " << font.get() << "):" << std::endl;
      }

      for (int gi=0; gi < run->glyphs->num_glyphs; gi++){
        result.emplace_back();

        // new run with single glyph
        std::shared_ptr<PangoGlyphString> singleGlyphStr = std::shared_ptr<PangoGlyphString>(pango_glyph_string_new(), pango_glyph_string_free);

        pango_glyph_string_set_size(singleGlyphStr.get(), 1);

        // make glyph copy
        singleGlyphStr->glyphs[0] = run->glyphs->glyphs[gi];
        PangoGlyphInfo &glyphInfo = singleGlyphStr->glyphs[0];

        result.back().glyph.font = font;


        result.back().position=Vertex2D(((double)glyphInfo.geometry.x_offset/(double)PANGO_SCALE) + horizontalOffset,
                                        (double)glyphInfo.geometry.y_offset/(double)PANGO_SCALE);

        if constexpr (debugLabelLayouter) {
          std::cout << "     " << glyphInfo.glyph << ": " << result.back().position.GetX() << " x "
                    << result.back().position.GetY() << std::endl;
        }

        glyphInfo.geometry.x_offset = 0;
        glyphInfo.geometry.y_offset = 0;
        // TODO: it is correct to take x_offset into account? See pango_glyph_string_extents_range implementation...
        horizontalOffset += ((double)(glyphInfo.geometry.width + glyphInfo.geometry.x_offset)/(double)PANGO_SCALE);

        result.back().glyph.glyphString = singleGlyphStr;
      }

      if (pango_layout_iter_next_run(iter) == 0){
        pango_layout_iter_free(iter);
        iter = nullptr;
      }
    }

    return result;
  }

  std::shared_ptr<MapPainterCairo::CairoLabel> MapPainterCairo::Layout(const Projection& projection,
                                                                       const MapParameter& parameter,
                                                                       const std::string& text,
                                                                       double fontSize,
                                                                       double objectWidth,
                                                                       bool enableWrapping,
                                                                       bool /*contourLabel*/)
  {
    auto label = std::make_shared<MapPainterCairo::CairoLabel>(
        std::shared_ptr<PangoLayout>(pango_cairo_create_layout(draw), g_object_unref));

    CairoFont font=GetFont(projection,
                           parameter,
                           fontSize);

    pango_layout_set_font_description(label->label.get(),font);

    int proposedWidth=(int)std::ceil(objectWidth);

    pango_layout_set_text(label->label.get(),
                          text.c_str(),
                          (int)text.length());

    // layout 0,0 coordinate will be top-center
    pango_layout_set_alignment(label->label.get(), PANGO_ALIGN_CENTER);

    if (enableWrapping) {
      pango_layout_set_wrap(label->label.get(), PANGO_WRAP_WORD);
    }

    if (proposedWidth > 0) {
      pango_layout_set_width(label->label.get(), proposedWidth * PANGO_SCALE);
    }

    PangoRectangle extends;

    pango_layout_get_pixel_extents(label->label.get(),
                                   nullptr,
                                   &extends);

    label->text=text;
    label->fontSize=fontSize;
    label->width=extends.width;
    label->height=extends.height;

    return label;
  }

  DoubleRectangle MapPainterCairo::GlyphBoundingBox(const CairoNativeGlyph &glyph) const
  {
    assert(glyph.glyphString->num_glyphs == 1);
    PangoRectangle extends;
    pango_font_get_glyph_extents(glyph.font.get(), glyph.glyphString->glyphs[0].glyph, nullptr, &extends);

    return DoubleRectangle((double)(extends.x) / (double)PANGO_SCALE,
                           (double)(extends.y) / (double)PANGO_SCALE,
                           (double)(extends.width) / (double)PANGO_SCALE,
                           (double)(extends.height) / (double)PANGO_SCALE);
  }

  void MapPainterCairo::DrawGlyphs(const Projection &/*projection*/,
                                   const MapParameter &/*parameter*/,
                                   const osmscout::PathTextStyleRef& style,
                                   const std::vector<CairoGlyph> &glyphs)
  {
    cairo_matrix_t matrix;
    cairo_get_matrix(draw, &matrix);

    cairo_set_source_rgba(draw,
                          style->GetTextColor().GetR(),
                          style->GetTextColor().GetG(),
                          style->GetTextColor().GetB(),
                          style->GetTextColor().GetA());

    for (auto const &glyph:glyphs) {
      cairo_set_matrix(draw, &matrix);
      cairo_translate(draw, glyph.position.GetX(), glyph.position.GetY());
      cairo_rotate(draw, glyph.angle);

      cairo_move_to(draw, 0, 0);
      pango_cairo_show_glyph_string(draw,
                                    glyph.glyph.font.get(),
                                    glyph.glyph.glyphString.get());
    }

    cairo_set_matrix(draw, &matrix);
    if constexpr (debugLabelLayouter) {
      for (auto const &glyph: glyphs) {
        cairo_set_source_rgba(draw, 1, 0, 0, 1);
        cairo_arc(draw,
                  glyph.trPosition.GetX(), glyph.trPosition.GetY(),
                  0.5,
                  0,
                  2 * M_PI);
        cairo_fill(draw);

        cairo_set_source_rgba(draw, 0, 0, 1, 1);
        cairo_set_line_width(draw, 0.2);
        cairo_rectangle(draw,
                        glyph.trPosition.GetX(), glyph.trPosition.GetY(),
                        glyph.trWidth, glyph.trHeight);
        cairo_stroke(draw);
      }
    }
  }


#else

  DoubleRectangle MapPainterCairo::GlyphBoundingBox(const CairoNativeGlyph &glyph) const
  {
    return DoubleRectangle(0,
                           glyph.height * -1,
                           glyph.width,
                           glyph.height);
  }

  template<>
  std::vector<Glyph<MapPainterCairo::CairoNativeGlyph>> MapPainterCairo::CairoLabel::ToGlyphs() const
  {
    std::vector<Glyph<MapPainterCairo::CairoNativeGlyph>> result;

    double horizontalOffset = 0;
    for (size_t ch = 0; ch < label.wstr.length(); ch++){
      result.emplace_back();

      result.back().glyph.character = WStringToUTF8String(label.wstr.substr(ch,1));

      cairo_text_extents_t  textExtents;
      cairo_scaled_font_text_extents(label.font,
                                     result.back().glyph.character.c_str(),
                                     &textExtents);

      result.back().glyph.width = textExtents.width;
      result.back().glyph.height = label.fontExtents.height;

      result.back().position.SetX(horizontalOffset);
      result.back().position.SetY(0);

      horizontalOffset += result.back().glyph.width;
    }

    return result;
  }

  void MapPainterCairo::DrawGlyphs(const Projection &/*projection*/,
                                   const MapParameter &/*parameter*/,
                                   const osmscout::PathTextStyleRef& style,
                                   const std::vector<CairoGlyph> &glyphs)
  {
    cairo_matrix_t matrix;
    cairo_get_matrix(draw, &matrix);

    cairo_set_source_rgba(draw,
                          style->GetTextColor().GetR(),
                          style->GetTextColor().GetG(),
                          style->GetTextColor().GetB(),
                          style->GetTextColor().GetA());

    for (auto const &glyph:glyphs) {

      cairo_set_matrix(draw, &matrix);
      cairo_translate(draw, glyph.position.GetX(), glyph.position.GetY());
      cairo_rotate(draw, glyph.angle);

      cairo_move_to(draw, 0, 0);
      cairo_show_text(draw,glyph.glyph.character.c_str());
      cairo_stroke(draw);
    }

    cairo_set_matrix(draw, &matrix);
  }

  std::shared_ptr<MapPainterCairo::CairoLabel> MapPainterCairo::Layout(const Projection& projection,
                                                                       const MapParameter& parameter,
                                                                       const std::string& text,
                                                                       double fontSize,
                                                                       double /*objectWidth*/,
                                                                       bool /*enableWrapping*/,
                                                                       bool /*contourLabel*/)
  {
    auto label = std::make_shared<MapPainterCairo::CairoLabel>();

    label->label.wstr = UTF8StringToWString(text);

    label->label.font = GetFont(projection, parameter, fontSize);

    cairo_scaled_font_extents(label->label.font,
                              &(label->label.fontExtents));

    cairo_scaled_font_text_extents(label->label.font,
                                   text.c_str(),
                                   &(label->label.textExtents));
    label->text=text;
    label->fontSize=fontSize;
    label->width=label->label.textExtents.width;
    label->height=label->label.fontExtents.height;

    return label;
  }

#endif

  void MapPainterCairo::DrawLabel(const Projection &/*projection*/,
                                  const MapParameter &/*parameter*/,
                                  const DoubleRectangle &labelRectangle,
                                  const LabelData &label,
                                  const CairoNativeLabel &layout)
  {
    if (const auto *style = dynamic_cast<const TextStyle *>(label.style.get());
        style != nullptr) {

      double r = style->GetTextColor().GetR();
      double g = style->GetTextColor().GetG();
      double b = style->GetTextColor().GetB();

      cairo_set_source_rgba(draw, r, g, b, label.alpha);

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
      PangoRectangle extends;

      pango_layout_get_pixel_extents(layout.get(),
                                     nullptr,
                                     &extends);

      cairo_move_to(draw,
                    labelRectangle.x - extends.x,
                    labelRectangle.y);

      if (style->GetStyle() == TextStyle::normal) {
        pango_cairo_show_layout(draw,
                                layout.get());
        cairo_stroke(draw);
      }
      else /* emphasize */ {

        double er = style->GetEmphasizeColor().GetR();
        double eg = style->GetEmphasizeColor().GetG();
        double eb = style->GetEmphasizeColor().GetB();

        pango_cairo_layout_path(draw,
                                layout.get());

        cairo_set_source_rgba(draw, er, eg, eb, label.alpha);
        cairo_set_line_width(draw, 2.0);
        cairo_stroke_preserve(draw);

        cairo_set_source_rgba(draw, r, g, b, label.alpha);
        cairo_fill(draw);
      }
#else
      cairo_move_to(draw,
                    labelRectangle.x,
                    labelRectangle.y+layout.fontExtents.ascent);

      if (style->GetStyle()==TextStyle::normal) {
        cairo_show_text(draw,label.text.c_str());
        cairo_stroke(draw);
      }
      else {
        cairo_text_path(draw,label.text.c_str());

        cairo_set_source_rgba(draw,1,1,1,label.alpha);
        cairo_set_line_width(draw,2.0);
        cairo_stroke_preserve(draw);

        cairo_set_source_rgba(draw,r,g,b,label.alpha);
        cairo_fill(draw);
      }

#endif

    }
    else if (const auto* style = dynamic_cast<const ShieldStyle*>(label.style.get());
             style != nullptr) {

      cairo_set_dash(draw,nullptr,0,0);
      cairo_set_line_width(draw,1);
      cairo_set_source_rgba(draw,
                            style->GetBgColor().GetR(),
                            style->GetBgColor().GetG(),
                            style->GetBgColor().GetB(),
                            style->GetBgColor().GetA());

      cairo_rectangle(draw,
                      labelRectangle.x-2,
                      labelRectangle.y,
                      labelRectangle.width+3,
                      labelRectangle.height+1);
      cairo_fill(draw);

      cairo_set_source_rgba(draw,
                            style->GetBorderColor().GetR(),
                            style->GetBorderColor().GetG(),
                            style->GetBorderColor().GetB(),
                            style->GetBorderColor().GetA());

      cairo_rectangle(draw,
                      labelRectangle.x +0,
                      labelRectangle.y +2,
                      labelRectangle.width +3-4,
                      labelRectangle.height +1-4);
      cairo_stroke(draw);

      cairo_set_source_rgba(draw,
                            style->GetTextColor().GetR(),
                            style->GetTextColor().GetG(),
                            style->GetTextColor().GetB(),
                            label.alpha);

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
      PangoRectangle extends;

      pango_layout_get_pixel_extents(layout.get(),
                                     nullptr,
                                     &extends);

      cairo_move_to(draw,
                    labelRectangle.x - extends.x,
                    labelRectangle.y);

      pango_cairo_show_layout(draw,
                              layout.get());
      cairo_stroke(draw);

#else
      cairo_move_to(draw,
                    labelRectangle.x,
                    labelRectangle.y+layout.fontExtents.ascent);

      cairo_show_text(draw,label.text.c_str());
      cairo_stroke(draw);

#endif
    }

  }

  void MapPainterCairo::RegisterRegularLabel(const Projection &projection,
                                             const MapParameter &parameter,
                                             const std::vector<LabelData> &labels,
                                             const Vertex2D &position,
                                             double objectWidth)
  {
    labelLayouter.RegisterLabel(projection,
                                parameter,
                                position,
                                labels,
                                objectWidth);
  }

  /**
   * Register contour label
   */
  void MapPainterCairo::RegisterContourLabel(const Projection &projection,
                                    const MapParameter &parameter,
                                    const PathLabelData &label,
                                    const LabelPath &labelPath)
  {
    labelLayouter.RegisterContourLabel(projection,
                                       parameter,
                                       label,
                                       labelPath);
  }

  void MapPainterCairo::DrawLabels(const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& /*data*/)
  {
    labelLayouter.Layout(projection, parameter);

    labelLayouter.DrawLabels(projection,
                             parameter,
                             this);

    labelLayouter.Reset();
  }

  void MapPainterCairo::DrawSymbol(const Projection& projection,
                                   const MapParameter& /*parameter*/,
                                   const Symbol& symbol,
                                   const Vertex2D& screenPos,
                                   double scaleFactor)
  {
    SymbolRendererCairo renderer(draw);

    renderer.Render(projection,
                    symbol,
                    screenPos,
                    scaleFactor);
  }

  void MapPainterCairo::DrawIcon(const IconStyle* style,
                                 const Vertex2D& centerPos,
                                 double width, double height)
  {
    size_t idx=style->GetIconId()-1;

    assert(idx<images.size());
    assert(images[idx]!=nullptr);

    cairo_surface_t *icon = images[idx];
    int w = cairo_image_surface_get_width(icon);
    int h = cairo_image_surface_get_height(icon);

    cairo_matrix_t matrix;
    cairo_get_matrix(draw, &matrix);
    double scaleW = width/w;
    double scaleH = height/h;
    cairo_scale(draw, scaleW, scaleH);

    cairo_set_source_surface(draw,
                             icon,
                             (centerPos.GetX()-width/2) / scaleW,
                             (centerPos.GetY()-height/2) / scaleH);

    cairo_paint(draw);
    cairo_set_matrix(draw, &matrix);
  }

  void MapPainterCairo::DrawPath(const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const Color& color,
                                 double width,
                                 const std::vector<double>& dash,
                                 LineStyle::CapStyle startCap,
                                 LineStyle::CapStyle endCap,
                                 const CoordBufferRange& coordRange)
  {
    SetLineAttributes(color,width,dash);

    if (startCap==LineStyle::capButt ||
       endCap==LineStyle::capButt) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
    }
    else if (startCap==LineStyle::capSquare ||
             endCap==LineStyle::capSquare) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_SQUARE);
    }
    else {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
    }

    cairo_new_path(draw);
    cairo_move_to(draw,
                  coordRange.GetFirst().GetX(),
                  coordRange.GetFirst().GetY());

    for (size_t i=coordRange.GetStart()+1; i<=coordRange.GetEnd(); i++) {
      cairo_line_to(draw,
                    coordRange.Get(i).GetX(),
                    coordRange.Get(i).GetY());
    }

    cairo_stroke(draw);

    if ((startCap==LineStyle::capRound || endCap==LineStyle::capRound) &&
        cairo_get_line_cap(draw)!=CAIRO_LINE_CAP_ROUND)
    {
      if (startCap==LineStyle::capRound) {
        cairo_new_path(draw);
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
        cairo_set_dash(draw,nullptr,0,0);
        cairo_set_line_width(draw,width);

        cairo_move_to(draw,
                      coordRange.GetFirst().GetX(),
                      coordRange.GetFirst().GetY());
        cairo_line_to(draw,
                      coordRange.GetFirst().GetX(),
                      coordRange.GetFirst().GetY());
        cairo_stroke(draw);
      }

      if (endCap==LineStyle::capRound) {
        cairo_new_path(draw);
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
        cairo_set_dash(draw,nullptr,0,0);
        cairo_set_line_width(draw,width);

        cairo_move_to(draw,
                      coordRange.GetLast().GetX(),
                      coordRange.GetLast().GetY());
        cairo_line_to(draw,
                      coordRange.GetLast().GetX(),
                      coordRange.GetLast().GetY());
        cairo_stroke(draw);
      }
    }
  }

  void MapPainterCairo::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapPainter::AreaData& area)
  {
    cairo_save(draw);

    if (!area.clippings.empty()) {
      cairo_set_fill_rule (draw,CAIRO_FILL_RULE_EVEN_ODD);
    }

    cairo_new_path(draw);
    cairo_move_to(draw,
                  area.coordRange.GetFirst().GetX(),
                  area.coordRange.GetFirst().GetY());
    for (size_t i=area.coordRange.GetStart()+1; i<=area.coordRange.GetEnd(); i++) {
      cairo_line_to(draw,
                    area.coordRange.Get(i).GetX(),
                    area.coordRange.Get(i).GetY());
    }
    cairo_close_path(draw);

    if (!area.clippings.empty()) {
      // Clip areas within the area by using CAIRO_FILL_RULE_EVEN_ODD
      for (const auto& data : area.clippings) {
        cairo_new_sub_path(draw);
        cairo_set_line_width(draw,0.0);
        cairo_move_to(draw,
                      data.GetFirst().GetX(),
                      data.GetFirst().GetY());
        for (size_t i=data.GetStart()+1; i<=data.GetEnd(); i++) {
          cairo_line_to(draw,
                        data.Get(i).GetX(),
                        data.Get(i).GetY());
        }
        cairo_close_path(draw);
      }
    }

    DrawFillStyle(projection,
                  parameter,
                  area.fillStyle,
                  area.borderStyle);

    cairo_restore(draw);
  }

  void MapPainterCairo::DrawGround(const Projection& projection,
                                   const MapParameter& /*parameter*/,
                                   const FillStyle& style)
  {
    cairo_set_source_rgba(draw,
                          style.GetFillColor().GetR(),
                          style.GetFillColor().GetG(),
                          style.GetFillColor().GetB(),
                          1);

    cairo_rectangle(draw,
                    0,
                    0,
                    projection.GetWidth(),
                    projection.GetHeight());
    cairo_fill(draw);
  }

  bool MapPainterCairo::DrawMap(const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data,
                                cairo_t *draw,
                                RenderSteps startStep,
                                RenderSteps endStep)
  {
    std::lock_guard<std::mutex> guard(mutex);

    this->draw=draw;

    minimumLineWidth=parameter.GetLineMinWidthPixel()*25.4/projection.GetDPI();

    return Draw(projection,
                parameter,
                data,
                startStep,
                endStep);
  }
}
