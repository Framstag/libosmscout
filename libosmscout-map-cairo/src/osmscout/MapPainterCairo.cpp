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

#include <osmscout/MapPainterCairo.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <list>

#include <osmscout/LoaderPNG.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  /* Returns Euclidean distance between two points */
  static double CalculatePointDistance(cairo_path_data_t *a, cairo_path_data_t *b)
  {
    double dx=b->point.x-a->point.x;
    double dy=b->point.y-a->point.y;

    return sqrt(dx*dx+dy*dy);
  }

  /**
   * Calculate an array of double for the path, that contains the length of each path segment
   */
  static double* CalculatePathSegmentLengths(cairo_path_t *path)
  {
    cairo_path_data_t *startPoint=nullptr;
    cairo_path_data_t *currentPoint=nullptr;
    double            *parametrization;

    parametrization=new double[path->num_data];

    for (int i=0; i < path->num_data; i += path->data[i].header.length) {
      cairo_path_data_t *data=&path->data[i];

      parametrization[i] = 0.0;

      switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO:
        startPoint = &data[1];
        currentPoint = &data[1];
        break;
      case CAIRO_PATH_CLOSE_PATH:
        // From current point back to the start */
        parametrization[i] = CalculatePointDistance(currentPoint,startPoint);
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
  static void PathPointTransformer(double& x,
                                   double& y,
                                   cairo_path_t *path,
                                   const double *pathSegmentLengths)
  {
    int               i;
    double            the_y=y;
    double            the_x=x;
    cairo_path_data_t *data;
    cairo_path_data_t *startPoint=nullptr;
    cairo_path_data_t *currentPoint=nullptr;

    // Find the segment on the line that is "x" away from the start
    for (i=0;
         i+path->data[i].header.length < path->num_data &&
         (the_x > pathSegmentLengths[i] ||
          path->data[i].header.type == CAIRO_PATH_MOVE_TO);
         i+=path->data[i].header.length) {
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
    case CAIRO_PATH_CLOSE_PATH:
      {
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
    case CAIRO_PATH_LINE_TO:
      {
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
    for (int i=0; i<srcPath->num_data; i+=srcPath->data[i].header.length) {
      cairo_path_data_t *data=&srcPath->data[i];

      switch (data->header.type) {
      case CAIRO_PATH_CURVE_TO:
        data[1].point.x+=xOffset;
        data[1].point.y+=yOffset;
        data[2].point.x+=xOffset;
        data[2].point.y+=yOffset;
        data[3].point.x+=xOffset;
        data[3].point.y+=yOffset;
        PathPointTransformer(data[1].point.x, data[1].point.y,dstPath,pathSegmentLengths);
        PathPointTransformer(data[2].point.x, data[2].point.y,dstPath,pathSegmentLengths);
        PathPointTransformer(data[3].point.x, data[3].point.y,dstPath,pathSegmentLengths);
        break;
      case CAIRO_PATH_MOVE_TO:
        data[1].point.x+=xOffset;
        data[1].point.y+=yOffset;
        PathPointTransformer(data[1].point.x, data[1].point.y,dstPath,pathSegmentLengths);
        break;
      case CAIRO_PATH_LINE_TO:
        data[1].point.x+=xOffset;
        data[1].point.y+=yOffset;
        PathPointTransformer(data[1].point.x, data[1].point.y,dstPath,pathSegmentLengths);
        break;
      case CAIRO_PATH_CLOSE_PATH:
        break;
      default:
        assert(false);
        break;
      }
    }
  }


  /* Projects the current text path of cr onto the provided path. */
  static void MapPathOnPath(cairo_t *draw,
                            cairo_path_t *srcPath,
                            cairo_path_t *dstPath,
                            double xOffset,
                            double yOffset)
  {
    double *segmentLengths=CalculatePathSegmentLengths(dstPath);

    // Center text on path
    TransformPathOntoPath(srcPath,
                          dstPath,
                          segmentLengths,
                          xOffset,
                          yOffset);

    cairo_new_path(draw);
    cairo_append_path(draw,srcPath);

    delete [] segmentLengths;
  }


#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
  static void DrawContourLabelPangoCairo(cairo_t *draw,
                                         cairo_path_t *path,
                                         double offset,
                                         PangoLayout *layout,
                                         const PangoRectangle& extends)
  {
    // Create a new path for the text we should draw along the curve
    cairo_new_path(draw);
    pango_cairo_layout_path(draw,layout);

    cairo_path_t *textPath=cairo_copy_path(draw);

    // Now transform the text path so that it maps to the contour of the line
    MapPathOnPath(draw,
                  textPath,
                  path,
                  offset,
                  -0.5*extends.height);

    cairo_path_destroy(textPath);

    // Draw the text path
    cairo_fill(draw);
  }

#else
  static void DrawContourLabelCairo(cairo_t *draw,
                                    cairo_path_t *path,
                                    double offset,
                                    double textHeight,
                                    const std::string& text)
  {
    // Create a new path for the text we should draw along the curve
    cairo_new_path(draw);
    cairo_move_to(draw,0,0);
    cairo_text_path(draw,text.c_str());

    cairo_path_t *textPath=cairo_copy_path(draw);

    // Now transform the text path so that it maps to the contour of the line
    MapPathOnPath(draw,
                  textPath,
                  path,
                  offset,
                  0.5*textHeight);

    cairo_path_destroy(textPath);

    // Draw the text path
    cairo_fill(draw);
  }
#endif

  MapPainterCairo::MapPainterCairo(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig,
               new CoordBuffer())
  {
    // no code
  }

  MapPainterCairo::~MapPainterCairo()
  {
    for (const auto& image : images) {
      if (image!=nullptr) {
        cairo_surface_destroy(image);
      }
    }

    for (const auto& pattern : patterns) {
      if (pattern!=nullptr) {
        cairo_pattern_destroy(pattern);
      }
    }

    for (const auto& image : patternImages) {
      if (image!=nullptr) {
        cairo_surface_destroy(image);
      }
    }

    for (const auto& entry : fonts) {
      if (entry.second!=NULL) {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
        pango_font_description_free(entry.second);
#else
        cairo_scaled_font_destroy(entry->second);
#endif
      }
    }
  }

  MapPainterCairo::Font MapPainterCairo::GetFont(const Projection& projection,
                                                 const MapParameter& parameter,
                                                 double fontSize)
  {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    FontMap::const_iterator f;

    fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    PangoFontDescription* font=pango_font_description_new();

    pango_font_description_set_family(font,parameter.GetFontName().c_str());
    pango_font_description_set_absolute_size(font,fontSize*PANGO_SCALE);

    return fonts.insert(std::make_pair(fontSize,font)).first->second;
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

  void MapPainterCairo::SetLineAttributes(const Color& color,
                                          double width,
                                          const std::vector<double>& dash)
  {
    double dashArray[10];

    assert(dash.size()<=10);

    cairo_set_source_rgba(draw,
                          color.GetR(),
                          color.GetG(),
                          color.GetB(),
                          color.GetA());

    cairo_set_line_width(draw,width);

    if (dash.empty()) {
      cairo_set_dash(draw,nullptr,0,0);
    }
    else {
      for (size_t i=0; i<dash.size(); i++) {
        dashArray[i]=dash[i]*width;
      }
      cairo_set_dash(draw,dashArray,dash.size(),0);
    }
  }

  void MapPainterCairo::DrawFillStyle(const Projection& projection,
                                      const MapParameter& parameter,
                                      const FillStyleRef& fill,
                                      const BorderStyleRef& border)
  {
    bool hasFill=false;
    bool hasBorder=false;

    if (fill) {
      if (fill->HasPattern() &&
          projection.GetMagnification()>=fill->GetPatternMinMag() &&
          HasPattern(parameter,*fill)) {
        size_t idx=fill->GetPatternId()-1;

        assert(idx<patterns.size());
        assert(patterns[idx]!=nullptr);

        cairo_set_source(draw,patterns[idx]);
        hasFill=true;
      }
      else if (fill->GetFillColor().IsVisible()) {
        Color color=fill->GetFillColor();
        cairo_set_source_rgba(draw,
                              color.GetR(),
                              color.GetG(),
                              color.GetB(),
                              color.GetA());
        hasFill=true;
      }
    }

    if (border) {
      hasBorder=border->GetWidth()>0 &&
                border->GetColor().IsVisible() &&
                border->GetWidth()>=minimumLineWidth;
    }

    if (hasFill && hasBorder) {
      cairo_fill_preserve(draw);
    }
    else if (hasFill) {
      cairo_fill(draw);
    }

    if (hasBorder) {
      double borderWidth=projection.ConvertWidthToPixel(border->GetWidth());

      if (borderWidth>=parameter.GetLineMinWidthPixel()) {
        SetLineAttributes(border->GetColor(),
                          borderWidth,
                          border->GetDash());

        cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);

        cairo_stroke(draw);
      }
    }
  }

  bool MapPainterCairo::HasIcon(const StyleConfig& /*styleConfig*/,
                                const MapParameter& parameter,
                                IconStyle& style)
  {
    // Already loaded with error
    if (style.GetIconId()==0) {
      return false;
    }

    size_t idx=style.GetIconId()-1;

    // Already cached?
    if (idx<images.size() &&
        images[idx]!=nullptr) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
         path!=parameter.GetIconPaths().end();
         ++path) {
      std::string filename=*path+style.GetIconName()+".png";

      cairo_surface_t *image=osmscout::LoadPNG(filename);

      if (image!=nullptr) {
        if (idx>=images.size()) {
          images.resize(idx+1,nullptr);
        }

        images[idx]=image;

        std::cout << "Loaded image '" << filename << "'" << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading image '" << style.GetIconName() << "'" << std::endl;
    style.SetIconId(0);

    return false;
  }

  bool MapPainterCairo::HasPattern(const MapParameter& parameter,
                                   const FillStyle& style)
  {
    assert(style.HasPattern()) ;

    // Already loaded with error
    if (style.GetPatternId()==0) {
      return false;
    }

    size_t idx=style.GetPatternId()-1;

    // Already cached?
    if (idx<patterns.size() &&
        patterns[idx]!=nullptr) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+style.GetPatternName()+".png";

      cairo_surface_t *image=osmscout::LoadPNG(filename);

      if (image!=nullptr) {
        if (idx>=patternImages.size()) {
          patternImages.resize(idx+1,nullptr);
        }

        patternImages[idx]=image;

        if (idx>=patterns.size()) {
          patterns.resize(idx+1,nullptr);
        }

        patterns[idx]=cairo_pattern_create_for_surface(patternImages[idx]);
        cairo_pattern_set_extend(patterns[idx],CAIRO_EXTEND_REPEAT);

        cairo_matrix_t matrix;

        cairo_matrix_init_scale(&matrix,1,1);
        cairo_pattern_set_matrix(patterns[idx],&matrix);

        std::cout << "Loaded pattern image '" << filename << "'" << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading pattern image '" << style.GetPatternName() << "'" << std::endl;
    style.SetPatternId(0);

    return false;
  }

  double MapPainterCairo::GetFontHeight(const Projection& projection,
                                      const MapParameter& parameter,
                                      double fontSize)
  {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    Font           font;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    return pango_font_description_get_size(font)/PANGO_SCALE;
#else
    Font                 font;
    cairo_font_extents_t fontExtents;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    cairo_scaled_font_extents(font,&fontExtents);

    return fontExtents.height;
#endif
  }

  MapPainter::TextDimension MapPainterCairo::GetTextDimension(const Projection& projection,
                                                              const MapParameter& parameter,
                                                              double objectWidth,
                                                              double fontSize,
                                                              const std::string& text)
  {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    Font             font=GetFont(projection,
                                  parameter,
                                  fontSize);
    PangoLayout      *layout=pango_cairo_create_layout(draw);

    pango_layout_set_font_description(layout,font);

    PangoContext     *context=pango_layout_get_context(layout);
    PangoFontMetrics *metrics=pango_context_get_metrics(context,
                                                        font,
                                                        pango_context_get_language(context));

    PangoRectangle   extends;

    pango_layout_set_text(layout,
                          text.c_str(),
                          (int)text.length());
    // Do not set an alignment here, since this will set toe X coordinate of
    // the extends to the center of the text. Since we do "center of" calculation already
    // in the MapPainter, this will result in broken coordinates.
    pango_layout_set_wrap(layout,PANGO_WRAP_WORD);

    if (objectWidth>0) {
      double proposedWidth=GetProposedLabelWidth(parameter,
                                                 pango_font_metrics_get_approximate_char_width(metrics)/PANGO_SCALE,
                                                 objectWidth,
                                                 text.length());

      pango_layout_set_width(layout,
                             (int)(proposedWidth*PANGO_SCALE));
    }

    pango_layout_get_pixel_extents(layout,
                                   &extends,
                                   nullptr);

    TextDimension dimension;

    if (pango_layout_get_line_count(layout)<=1) {
      dimension=TextDimension(extends.x,
                              extends.y,
                              extends.width,
                              pango_font_description_get_size(font)/PANGO_SCALE);

    }
    else {
      dimension=TextDimension(extends.x,
                              extends.y,
                              extends.width,
                              extends.height);
    }

    pango_font_metrics_unref(metrics);
    g_object_unref(layout);

    return dimension;
#else
    Font                 font;
    cairo_text_extents_t textExtents;
    cairo_font_extents_t fontExtents;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    cairo_scaled_font_extents(font,&fontExtents);

    cairo_scaled_font_text_extents(font,
                                   text.c_str(),
                                   &textExtents);

    return TextDimension(textExtents.x_bearing,
                         textExtents.y_bearing,
                         textExtents.width,
                         fontExtents.height);
#endif
  }

  void MapPainterCairo::DrawContourSymbol(const Projection& projection,
                                          const MapParameter& parameter,
                                          const Symbol& symbol,
                                          double space,
                                          size_t transStart, size_t transEnd)
  {
    double lineLength=0;
    double xo=0;
    double yo=0;

    cairo_save(draw);

    cairo_new_path(draw);

    for (size_t j=transStart; j<=transEnd; j++) {
      if (j==transStart) {
        cairo_move_to(draw,
                      coordBuffer->buffer[j].GetX(),
                      coordBuffer->buffer[j].GetY());
      }
      else {
        cairo_line_to(draw,
                      coordBuffer->buffer[j].GetX(),
                      coordBuffer->buffer[j].GetY());
        lineLength+=sqrt(pow(coordBuffer->buffer[j].GetX()-xo,2)+
                         pow(coordBuffer->buffer[j].GetY()-yo,2));
      }

      xo=coordBuffer->buffer[j].GetX();
      yo=coordBuffer->buffer[j].GetY();
    }

    cairo_path_t *path=cairo_copy_path_flat(draw);

    double minX;
    double minY;
    double maxX;
    double maxY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    double width=projection.ConvertWidthToPixel(maxX-minX);
    double height=projection.ConvertWidthToPixel(maxY-minY);

    for (const auto& primitive : symbol.GetPrimitives()) {
      FillStyleRef   fillStyle=primitive->GetFillStyle();
      BorderStyleRef borderStyle=primitive->GetBorderStyle();

      double offset=space/2.0;

      cairo_new_path(draw);

      while (offset+width<lineLength) {
        DrawPrimitivePath(projection,
                          parameter,
                          primitive,
                          offset+width/2,0,
                          minX,
                          minY,
                          maxX,
                          maxY);

        offset+=width+space;
      }

      cairo_path_t *patternPath=cairo_copy_path_flat(draw);

      // Now transform the text path so that it maps to the contour of the line
      MapPathOnPath(draw,
                    patternPath,
                    path,
                    0,
                    height/2);

      DrawFillStyle(projection,
                    parameter,
                    fillStyle,
                    borderStyle);

      cairo_path_destroy(patternPath);
    }

    cairo_path_destroy(path);

    cairo_restore(draw);
  }

  void MapPainterCairo::DrawLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const LabelData& label)
  {
    if (dynamic_cast<const TextStyle*>(label.style.get())!=nullptr) {
      const auto* style=dynamic_cast<const TextStyle*>(label.style.get());
      double           r=style->GetTextColor().GetR();
      double           g=style->GetTextColor().GetG();
      double           b=style->GetTextColor().GetB();
      Font             font=GetFont(projection,
                                    parameter,
                                    label.fontSize);

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
      PangoLayout      *layout=pango_cairo_create_layout(draw);

      pango_layout_set_font_description(layout,font);

      PangoContext     *context=pango_layout_get_context(layout);
      PangoFontMetrics *metrics=pango_context_get_metrics(context,
                                                          font,
                                                          pango_context_get_language(context));
      int              proposedWidth=(int)std::ceil(label.bx2-label.bx1);

      pango_layout_set_text(layout,
                            label.text.c_str(),
                            (int)label.text.length());
      pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);
      pango_layout_set_wrap(layout,PANGO_WRAP_WORD);

      pango_layout_set_width(layout,proposedWidth*PANGO_SCALE);

      PangoRectangle extends;

      pango_layout_get_pixel_extents(layout,
                                     nullptr,
                                     &extends);

      cairo_set_source_rgba(draw,r,g,b,label.alpha);

      cairo_move_to(draw,
                    label.x,
                    label.y);

      if (style->GetStyle()==TextStyle::normal) {
        pango_cairo_show_layout(draw,
                                layout);
        cairo_stroke(draw);
      }
      else /* emphasize */ {
        pango_cairo_layout_path(draw,
                                layout);

        cairo_set_source_rgba(draw,1,1,1,label.alpha);
        cairo_set_line_width(draw,2.0);
        cairo_stroke_preserve(draw);

        cairo_set_source_rgba(draw,r,g,b,label.alpha);
        cairo_fill(draw);
      }

      pango_font_metrics_unref(metrics);
      g_object_unref(layout);

#else
      cairo_font_extents_t fontExtents;

      cairo_set_scaled_font(draw,font);

      cairo_scaled_font_extents(font,&fontExtents);

      cairo_set_source_rgba(draw,r,g,b,label.alpha);

      cairo_move_to(draw,
                    label.x,
                    label.y+fontExtents.ascent);

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
    else if (dynamic_cast<const ShieldStyle*>(label.style.get())!=nullptr) {
      const auto* style=dynamic_cast<const ShieldStyle*>(label.style.get());

      cairo_set_dash(draw,nullptr,0,0);
      cairo_set_line_width(draw,1);
      cairo_set_source_rgba(draw,
                            style->GetBgColor().GetR(),
                            style->GetBgColor().GetG(),
                            style->GetBgColor().GetB(),
                            style->GetBgColor().GetA());

      cairo_rectangle(draw,
                      label.bx1,
                      label.by1,
                      label.bx2-label.bx1+1,
                      label.by2-label.by1+1);
      cairo_fill(draw);

      cairo_set_source_rgba(draw,
                            style->GetBorderColor().GetR(),
                            style->GetBorderColor().GetG(),
                            style->GetBorderColor().GetB(),
                            style->GetBorderColor().GetA());

      cairo_rectangle(draw,
                      label.bx1+2,
                      label.by1+2,
                      label.bx2-label.bx1+1-4,
                      label.by2-label.by1+1-4);
      cairo_stroke(draw);

      cairo_set_source_rgba(draw,
                            style->GetTextColor().GetR(),
                            style->GetTextColor().GetG(),
                            style->GetTextColor().GetB(),
                            label.alpha);
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
      Font        font=GetFont(projection,
                               parameter,
                               label.fontSize);
      PangoLayout *layout=pango_cairo_create_layout(draw);

      pango_layout_set_font_description(layout,font);
      pango_layout_set_text(layout,label.text.c_str(),
                            (int)label.text.length());

      cairo_move_to(draw,
                    label.x,
                    label.y);

      pango_cairo_show_layout(draw,
                              layout);
      cairo_stroke(draw);

      g_object_unref(layout);

#else
      Font                 font=GetFont(projection,
                                        parameter,
                                        label.fontSize);
      cairo_font_extents_t fontExtents;

      cairo_set_scaled_font(draw,font);

      cairo_scaled_font_extents(font,&fontExtents);

      cairo_move_to(draw,
                    label.x,
                    label.y+(label.by2-label.by1-fontExtents.height)/2);

      cairo_show_text(draw,label.text.c_str());
      cairo_stroke(draw);
#endif
    }
  }

  void MapPainterCairo::DrawContourLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const PathTextStyle& style,
                                         const std::string& text,
                                         size_t transStart, size_t transEnd,
                                         ContourLabelHelper& helper)
  {
    double pathLength=0;

    // Make the way path known to cairo and at the same time calculate the length
    // of the path

    cairo_new_path(draw);

    if (coordBuffer->buffer[transStart].GetX()<=coordBuffer->buffer[transEnd].GetX()) {
      // coordinates of previous point
      double xo=0;
      double yo=0;

      for (size_t j=transStart; j<=transEnd; j++) {
        if (j==transStart) {
          cairo_move_to(draw,
                        coordBuffer->buffer[j].GetX(),
                        coordBuffer->buffer[j].GetY());
        }
        else {
          cairo_line_to(draw,
                        coordBuffer->buffer[j].GetX(),
                        coordBuffer->buffer[j].GetY());
          pathLength+=sqrt(pow(coordBuffer->buffer[j].GetX()-xo,2)+
                           pow(coordBuffer->buffer[j].GetY()-yo,2));
        }

        xo=coordBuffer->buffer[j].GetX();
        yo=coordBuffer->buffer[j].GetY();
      }
    }
    else {
      // coordinates of previous point
      double xo=0;
      double yo=0;

      for (size_t j=0; j<=transEnd-transStart; j++) {
        size_t idx=transEnd-j;

        if (j==0) {
          cairo_move_to(draw,
                        coordBuffer->buffer[idx].GetX(),
                        coordBuffer->buffer[idx].GetY());
        }
        else {
          cairo_line_to(draw,
                        coordBuffer->buffer[idx].GetX(),
                        coordBuffer->buffer[idx].GetY());
          pathLength+=sqrt(pow(coordBuffer->buffer[idx].GetX()-xo,2)+
                           pow(coordBuffer->buffer[idx].GetY()-yo,2));
        }

        xo=coordBuffer->buffer[idx].GetX();
        yo=coordBuffer->buffer[idx].GetY();
      }
    }

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    Font           font=GetFont(projection,
                                parameter,
                                style.GetSize());
    PangoLayout    *layout=pango_cairo_create_layout(draw);
    PangoRectangle extends;

    pango_layout_set_font_description(layout,font);
    pango_layout_set_text(layout,
                          text.c_str(),
                          text.length());
    pango_layout_get_pixel_extents(layout,&extends,nullptr);

    double textWidth=extends.width;

    if (!helper.Init(pathLength,
                     textWidth)) {
      g_object_unref(layout);
      return;
    }

    // Current offset for the next label
    cairo_path_t *path;

    /* Decrease tolerance a bit, since it's going to be magnified */
    //cairo_set_tolerance (cr, 0.01);

    /* Using cairo_copy_path() here shows our deficiency in handling
      * Bezier curves, specially around sharper curves.
      *
      * Using cairo_copy_path_flat() on the other hand, magnifies the
      * flattening error with large off-path values. We decreased
      * tolerance for that reason. Increase tolerance to see that
      * artifact.
    */

    // Make a copy of the path of the line we should draw along
    path=cairo_copy_path_flat(draw);

    cairo_set_source_rgba(draw,
                          style.GetTextColor().GetR(),
                          style.GetTextColor().GetG(),
                          style.GetTextColor().GetB(),
                          style.GetTextColor().GetA());

    while (helper.ContinueDrawing()) {
      DrawContourLabelPangoCairo(draw,
                                 path,
                                 helper.GetCurrentOffset()-extends.x,
                                 layout,
                                 extends);

      helper.AdvanceText();
      helper.AdvanceSpace();
    }

    cairo_path_destroy(path);
    g_object_unref(layout);

#else
    Font                 font=GetFont(projection,
                                      parameter,
                                      style.GetSize());
    cairo_text_extents_t textExtents;

    cairo_scaled_font_text_extents(font,
                                   text.c_str(),
                                   &textExtents);

    double textWidth=textExtends.width;

    if (!helper.Init(pathLength,
                     textWidth)) {
      g_object_unref(layout);
      return;
    }

    // Current offset for the next label
    double offset=helper.GetInitialOffset(pathLength,
                                          textWidth);

    cairo_font_extents_t fontExtents;
    cairo_path_t         *path;

    cairo_scaled_font_extents(font,
                              &fontExtents);

    path=cairo_copy_path_flat(draw);

    cairo_set_source_rgba(draw,
                          style.GetTextColor().GetR(),
                          style.GetTextColor().GetG(),
                          style.GetTextColor().GetB(),
                          style.GetTextColor().GetA());

    cairo_set_scaled_font(draw,font);

    while (offset<lineLength) {
      DrawContourLabelCairo(draw,
                            path,
                            offset-textExtents.x_bearing,
                            textExtents.height,
                            text);

      offset=offset+textWidth+contourLabelSpace;
    }

    cairo_path_destroy(path);
#endif
  }

  void MapPainterCairo::DrawPrimitivePath(const Projection& projection,
                                          const MapParameter& /*parameter*/,
                                          const DrawPrimitiveRef& p,
                                          double x, double y,
                                          double minX,
                                          double minY,
                                          double maxX,
                                          double maxY)
  {
    DrawPrimitive* primitive=p.get();
    double         centerX=(minX+maxX)/2;
    double         centerY=(minY+maxY)/2;

    if (dynamic_cast<PolygonPrimitive*>(primitive)!=nullptr) {
      const auto* polygon=dynamic_cast<const PolygonPrimitive*>(primitive);

      for (auto pixel=polygon->GetCoords().begin();
           pixel!=polygon->GetCoords().end();
           ++pixel) {
        if (pixel==polygon->GetCoords().begin()) {
          cairo_move_to(draw,
                        x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                        y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
        }
        else {
          cairo_line_to(draw,
                        x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                        y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
        }
      }

      cairo_close_path(draw);
    }
    else if (dynamic_cast<RectanglePrimitive*>(primitive)!=nullptr) {
      const auto* rectangle=dynamic_cast<const RectanglePrimitive*>(primitive);

      cairo_rectangle(draw,
                      x+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX()-centerX),
                      y+projection.ConvertWidthToPixel(maxY-rectangle->GetTopLeft().GetY()-centerY),
                      projection.ConvertWidthToPixel(rectangle->GetWidth()),
                      projection.ConvertWidthToPixel(rectangle->GetHeight()));
    }
    else if (dynamic_cast<CirclePrimitive*>(primitive)!=nullptr) {
      const auto* circle=dynamic_cast<const CirclePrimitive*>(primitive);

      cairo_arc(draw,
                x+projection.ConvertWidthToPixel(circle->GetCenter().GetX()-centerX),
                y+projection.ConvertWidthToPixel(maxY-circle->GetCenter().GetY()-centerY),
                projection.ConvertWidthToPixel(circle->GetRadius()),
                0,2*M_PI);
    }
  }

  void MapPainterCairo::DrawSymbol(const Projection& projection,
                                   const MapParameter& parameter,
                                   const Symbol& symbol,
                                   double x, double y)
  {
    double minX;
    double minY;
    double maxX;
    double maxY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    for (const auto& primitive: symbol.GetPrimitives()) {
      FillStyleRef   fillStyle=primitive->GetFillStyle();
      BorderStyleRef borderStyle=primitive->GetBorderStyle();

      cairo_new_path(draw);

      DrawPrimitivePath(projection,
                        parameter,
                        primitive,
                        x,y,
                        minX,
                        minY,
                        maxX,
                        maxY);

      DrawFillStyle(projection,
                    parameter,
                    fillStyle,
                    borderStyle);
    }
  }

  void MapPainterCairo::DrawIcon(const IconStyle* style,
                                 double x, double y)
  {
    size_t idx=style->GetIconId()-1;

    assert(idx<images.size());
    assert(images[idx]!=nullptr);

    cairo_set_source_surface(draw,images[idx],x-7,y-7);
    cairo_paint(draw);
  }

  void MapPainterCairo::DrawPath(const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const Color& color,
                                 double width,
                                 const std::vector<double>& dash,
                                 LineStyle::CapStyle startCap,
                                 LineStyle::CapStyle endCap,
                                 size_t transStart, size_t transEnd)
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

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i==transStart) {
        cairo_new_path(draw);
        cairo_move_to(draw,
                      coordBuffer->buffer[i].GetX(),
                      coordBuffer->buffer[i].GetY());
      }
      else {
        cairo_line_to(draw,
                      coordBuffer->buffer[i].GetX(),
                      coordBuffer->buffer[i].GetY());
      }
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
                      coordBuffer->buffer[transStart].GetX(),
                      coordBuffer->buffer[transStart].GetY());
        cairo_line_to(draw,
                      coordBuffer->buffer[transStart].GetX(),
                      coordBuffer->buffer[transStart].GetY());
        cairo_stroke(draw);
      }

      if (endCap==LineStyle::capRound) {
        cairo_new_path(draw);
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
        cairo_set_dash(draw,nullptr,0,0);
        cairo_set_line_width(draw,width);

        cairo_move_to(draw,
                      coordBuffer->buffer[transEnd].GetX(),
                      coordBuffer->buffer[transEnd].GetY());
        cairo_line_to(draw,
                      coordBuffer->buffer[transEnd].GetX(),
                      coordBuffer->buffer[transEnd].GetY());
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
                  coordBuffer->buffer[area.transStart].GetX(),
                  coordBuffer->buffer[area.transStart].GetY());
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      cairo_line_to(draw,
                    coordBuffer->buffer[i].GetX(),
                    coordBuffer->buffer[i].GetY());
    }
    cairo_close_path(draw);

    if (!area.clippings.empty()) {
      // Clip areas within the area by using CAIRO_FILL_RULE_EVEN_ODD
      for (const auto& data : area.clippings) {
        cairo_new_sub_path(draw);
        cairo_set_line_width(draw,0.0);
        cairo_move_to(draw,
                      coordBuffer->buffer[data.transStart].GetX(),
                      coordBuffer->buffer[data.transStart].GetY());
        for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
          cairo_line_to(draw,
                        coordBuffer->buffer[i].GetX(),
                        coordBuffer->buffer[i].GetY());
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
                                cairo_t *draw)
  {
    std::lock_guard<std::mutex> guard(mutex);

    this->draw=draw;

    minimumLineWidth=parameter.GetLineMinWidthPixel()*25.4/projection.GetDPI();

    return Draw(projection,
                parameter,
                data);
  }
}

