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

#include <cassert>
#include <iostream>
#include <iomanip>
#include <limits>
#include <list>
#include <cstdlib>

#include <osmscout/LoaderPNG.h>
#include <osmscout/Util.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  /* Returns Euclidean distance between two points */
  static double CalculatePointDistance(cairo_path_data_t *a, cairo_path_data_t *b)
  {
    double dx=b->point.x-a->point.x;
    double dy=b->point.y-a->point.y;

    return sqrt(pow(dx,2)+pow(dy,2));
  }

  /**
   * Calculate an array of double for the path, that contains the length of each path segment
   */
  static double* CalculatePathSegmentLengths(cairo_path_t *path)
  {
    cairo_path_data_t *startPoint=NULL;
    cairo_path_data_t *currentPoint=NULL;
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


  typedef void (*transform_point_func_t)(void *transformData, double& x, double& y);

  /* Project a path using a function. Each point of the path (including
    * Bezier control points) is passed to the function for transformation.
  */
  static void TransformPath(cairo_path_t *path, transform_point_func_t f, void *transformData)
  {
    for (int i=0; i<path->num_data; i+=path->data[i].header.length) {
      cairo_path_data_t *data=&path->data[i];

      switch (data->header.type) {
      case CAIRO_PATH_CURVE_TO:
        f(transformData, data[1].point.x, data[1].point.y);
        f(transformData, data[2].point.x, data[2].point.y);
        f(transformData, data[3].point.x, data[3].point.y);
        break;
      case CAIRO_PATH_MOVE_TO:
        f(transformData, data[1].point.x, data[1].point.y);
        break;
      case CAIRO_PATH_LINE_TO:
        f(transformData, data[1].point.x, data[1].point.y);
        break;
      case CAIRO_PATH_CLOSE_PATH:
        break;
      default:
        assert(false);
        break;
      }
    }
  }


  /* Simple struct to hold a path and its parametrization */
  struct parametrized_path_t
  {
    cairo_path_t *path;
    double       *parametrization;
  };


  /* Project a point X,Y onto a parameterized path. The final point is
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
  static void PathPointTransformer(parametrized_path_t *param, double& x, double& y)
  {
    cairo_path_t      *path = param->path;
    double            *parametrization = param->parametrization;

    int               i;
    double            the_y=y;
    double            the_x=x;
    cairo_path_data_t *data;
    cairo_path_data_t *startPoint=NULL;
    cairo_path_data_t *currentPoint=NULL;

    // Find the segment on the line that is "x" away from the start
    for (i=0;
         i+path->data[i].header.length < path->num_data &&
         (the_x > parametrization[i] ||
          path->data[i].header.type == CAIRO_PATH_MOVE_TO);
         i+=path->data[i].header.length) {
      data = &path->data[i];

      the_x -= parametrization[i];

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
        break;
      case CAIRO_PATH_CLOSE_PATH:
        break;
      default:
        assert(false);
        break;
      }
    }

    data = &path->data[i];

    switch (data->header.type) {

    case CAIRO_PATH_MOVE_TO:
      break;
    case CAIRO_PATH_CLOSE_PATH:
      {
        // Relative offset in the current segment ([0..1])
        double ratio = the_x / parametrization[i];
        // Line polynomial
        x = currentPoint->point.x * (1 - ratio) + startPoint->point.x * ratio;
        y = currentPoint->point.y * (1 - ratio) + startPoint->point.y * ratio;

        // Line gradient
        double dx = -(currentPoint->point.x - startPoint->point.x);
        double dy = -(currentPoint->point.y - startPoint->point.y);

        // optimization for: ratio = the_y / sqrt (dx * dx + dy * dy)
        ratio = the_y / parametrization[i];
        x += -dy * ratio;
        y += dx * ratio;
      }
      break;
    case CAIRO_PATH_LINE_TO:
      {
        // Relative offset in the current segment ([0..1])
        double ratio = the_x / parametrization[i];
        // Line polynomial
        x = currentPoint->point.x * (1 - ratio) + data[1].point.x * ratio;
        y = currentPoint->point.y * (1 - ratio) + data[1].point.y * ratio;

        // Line gradient
        double dx = -(currentPoint->point.x - data[1].point.x);
        double dy = -(currentPoint->point.y - data[1].point.y);

        // optimization for: ratio = the_y / sqrt (dx * dx + dy * dy)
        ratio = the_y / parametrization[i];
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

  /* Projects the current path of cr onto the provided path. */
  static void MapCurrentPathOnContour(cairo_t *cr, cairo_path_t *path)
  {
    cairo_path_t        *textPath;
    parametrized_path_t param;

    param.path=path;
    param.parametrization=CalculatePathSegmentLengths(path);

    textPath=cairo_copy_path(cr);


    TransformPath(textPath,
                   (transform_point_func_t) PathPointTransformer,
                   &param);

    cairo_new_path(cr);
    cairo_append_path(cr,textPath);

    cairo_path_destroy(textPath);

    delete [] param.parametrization;
  }


  static void DrawContourLabelCairo(cairo_t *cr, double x, double y, const char *text)
  {
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
    path=cairo_copy_path_flat(cr);

    // Create a new path for the text we should draw along the curve
    cairo_new_path(cr);
    cairo_move_to(cr,x,y);
    cairo_text_path(cr,text);

    // Now transform the text path so that it maps to the contour of the line
    MapCurrentPathOnContour(cr,path);

    // Draw the text path
    cairo_fill(cr);

    cairo_path_destroy(path);
  }

  MapPainterCairo::MapPainterCairo()
  {
    // no code
  }

  MapPainterCairo::~MapPainterCairo()
  {
    for (std::vector<cairo_surface_t*>::iterator image=images.begin();
         image!=images.end();
         ++image) {
      if (*image!=NULL) {
        cairo_surface_destroy(*image);
      }
    }

    for (std::vector<cairo_pattern_t*>::iterator pattern=patterns.begin();
         pattern!=patterns.end();
         ++pattern) {
      if (*pattern!=NULL) {
        cairo_pattern_destroy(*pattern);
      }
    }

    for (std::map<size_t,cairo_scaled_font_t*>::const_iterator entry=font.begin();
         entry!=font.end();
         ++entry) {
      if (entry->second!=NULL) {
        cairo_scaled_font_destroy(entry->second);
      }
    }
  }

  cairo_scaled_font_t* MapPainterCairo::GetScaledFont(const MapParameter& parameter,
                                                      double fontSize)
  {
    std::map<size_t,cairo_scaled_font_t*>::const_iterator f;

    f=font.find(fontSize);

    if (f!=font.end()) {
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

    cairo_matrix_init_scale(&scaleMatrix,
                            parameter.GetFontSize()*fontSize,
                            parameter.GetFontSize()*fontSize);

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

    return font.insert(std::pair<size_t,cairo_scaled_font_t*>(fontSize,scaledFont)).first->second;
  }

  void MapPainterCairo::SetLineAttributes(double r, double g, double b, double a,
                                          double width,
                                          const std::vector<double>& dash)
  {
    double dashArray[10];

    assert(dash.size()<=10);

    cairo_set_source_rgba(draw,r,g,b,a);

    cairo_set_line_width(draw,width);

    if (dash.empty()) {
      cairo_set_dash(draw,NULL,0,0);
    }
    else {
      for (size_t i=0; i<dash.size(); i++) {
        dashArray[i]=dash[i]*width;
      }
      cairo_set_dash(draw,dashArray,dash.size(),0);
    }
  }

  bool MapPainterCairo::HasIcon(const StyleConfig& styleConfig,
                                const MapParameter& parameter,
                                IconStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
         path!=parameter.GetIconPaths().end();
         ++path) {
      std::string filename=*path+style.GetIconName()+".png";

      cairo_surface_t *image=osmscout::LoadPNG(filename);

      if (image!=NULL) {
        images.resize(images.size()+1,image);
        style.SetId(images.size());
        std::cout << "Loaded image " << filename << " => id " << style.GetId() << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading icon '" << style.GetIconName() << "'" << std::endl;
    style.SetId(std::numeric_limits<size_t>::max());

    return false;
  }

  bool MapPainterCairo::HasPattern(const MapParameter& parameter,
                                   const FillStyle& style)
  {
    assert(style.HasPattern()) ;

    // Was not able to load pattern
    if (style.GetPatternId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    // Pattern already loaded
    if (style.GetPatternId()!=0) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+style.GetPatternName()+".png";

      cairo_surface_t *image=osmscout::LoadPNG(filename);

      if (image!=NULL) {
        images.resize(images.size()+1,image);
        style.SetPatternId(images.size());
        patterns.resize(images.size(),NULL);

        patterns[patterns.size()-1]=cairo_pattern_create_for_surface(images[images.size()-1]);
        cairo_pattern_set_extend(patterns[patterns.size()-1],CAIRO_EXTEND_REPEAT);

        cairo_matrix_t matrix;

        cairo_matrix_init_scale(&matrix,1,1);
        cairo_pattern_set_matrix(patterns[patterns.size()-1],&matrix);

        std::cout << "Loaded image " << filename << " => id " << style.GetPatternId() << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading icon file '" << style.GetPatternName() << "'" << std::endl;
    style.SetPatternId(std::numeric_limits<size_t>::max());

    return false;
  }

  void MapPainterCairo::GetTextDimension(const MapParameter& parameter,
                                         double fontSize,
                                         const std::string& text,
                                         double& xOff,
                                         double& yOff,
                                         double& width,
                                         double& height)
  {
    cairo_scaled_font_t *font;
    cairo_text_extents_t textExtents;
    cairo_font_extents_t fontExtents;

    font=GetScaledFont(parameter,
                       fontSize);

    cairo_scaled_font_extents(font,&fontExtents);

    cairo_scaled_font_text_extents(font,
                                   text.c_str(),
                                   &textExtents);

    xOff=textExtents.x_bearing;
    yOff=textExtents.y_bearing;
    width=textExtents.width;
    height=fontExtents.height;
  }

  void MapPainterCairo::DrawLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const Label& label)
  {
    double               r=label.style->GetTextR();
    double               g=label.style->GetTextG();
    double               b=label.style->GetTextB();
    cairo_scaled_font_t  *font;
    cairo_font_extents_t fontExtents;

    font=GetScaledFont(parameter,
                       label.fontSize);

    cairo_set_scaled_font(draw,font);

    cairo_scaled_font_extents(font,&fontExtents);

    cairo_set_source_rgba(draw,r,g,b,label.alpha);

    cairo_move_to(draw,
                  label.x,
                  label.y+fontExtents.ascent);

    if (label.style->GetStyle()==LabelStyle::normal) {

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
  }

  void MapPainterCairo::DrawPlateLabel(const Projection& projection,
                                       const MapParameter& parameter,
                                       const Label& label)
  {
    cairo_scaled_font_t  *font;
    cairo_font_extents_t fontExtents;

    font=GetScaledFont(parameter,
                       label.fontSize);

    cairo_set_scaled_font(draw,font);

    cairo_scaled_font_extents(font,&fontExtents);

    cairo_set_dash(draw,NULL,0,0);
    cairo_set_line_width(draw,1);
    cairo_set_source_rgba(draw,
                          label.style->GetBgR(),
                          label.style->GetBgG(),
                          label.style->GetBgB(),
                          label.style->GetBgA());

    cairo_rectangle(draw,
                    label.bx,
                    label.by,
                    label.bwidth,
                    label.bheight);
    cairo_fill(draw);

    cairo_set_source_rgba(draw,
                          label.style->GetBorderR(),
                          label.style->GetBorderG(),
                          label.style->GetBorderB(),
                          label.style->GetBorderA());

    cairo_rectangle(draw,
                    label.bx+2,
                    label.by+2,
                    label.bwidth-4,
                    label.bheight-4);
    cairo_stroke(draw);

    cairo_set_source_rgba(draw,
                          label.style->GetTextR(),
                          label.style->GetTextG(),
                          label.style->GetTextB(),
                          label.style->GetTextA());

    cairo_move_to(draw,
                  label.x,
                  label.y+fontExtents.ascent);
    cairo_show_text(draw,label.text.c_str());
    cairo_stroke(draw);
  }

  void MapPainterCairo::DrawContourLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const LabelStyle& style,
                                         const std::string& text,
                                         const TransPolygon& contour)
  {
    assert(style.GetStyle()==LabelStyle::contour);

    cairo_scaled_font_t *font=GetScaledFont(parameter,
                                            style.GetSize());

    cairo_set_scaled_font(draw,font);

    double length=0;
    double xo=0;
    double yo=0;

    cairo_new_path(draw);

    if (contour.points[contour.GetStart()].x<=contour.points[contour.GetEnd()].x) {
      for (size_t j=contour.GetStart(); j<=contour.GetEnd(); j++) {
        if (contour.points[j].draw) {
          if (j==contour.GetStart()) {
            cairo_move_to(draw,
                          contour.points[j].x,
                          contour.points[j].y);
          }
          else {
            cairo_line_to(draw,
                          contour.points[j].x,
                          contour.points[j].y);
            length+=sqrt(pow(contour.points[j].x-xo,2)+
                         pow(contour.points[j].y-yo,2));
          }

          xo=contour.points[j].x;
          yo=contour.points[j].y;
        }
      }
    }
    else {
      for (size_t j=0; j<=contour.GetEnd()-contour.GetStart(); j++) {
        size_t idx=contour.GetEnd()-j;

        if (contour.points[idx].draw) {
          if (j==contour.GetStart()) {
            cairo_move_to(draw,
                          contour.points[idx].x,
                          contour.points[idx].y);
          }
          else {
            cairo_line_to(draw,
                          contour.points[idx].x,
                          contour.points[idx].y);
            length+=sqrt(pow(contour.points[idx].x-xo,2)+
                         pow(contour.points[idx].y-yo,2));
          }

          xo=contour.points[idx].x;
          yo=contour.points[idx].y;
        }
      }
    }

    cairo_text_extents_t textExtents;

    cairo_scaled_font_text_extents(font,
                                   text.c_str(),
                                   &textExtents);

    if (length<textExtents.width) {
      // Text is longer than path to draw on
      return;
    }

    cairo_font_extents_t fontExtents;

    cairo_scaled_font_extents(font,
                              &fontExtents);

    cairo_set_source_rgba(draw,
                          style.GetTextR(),
                          style.GetTextG(),
                          style.GetTextB(),
                          style.GetTextA());

    DrawContourLabelCairo(draw,
                          (length-textExtents.width)/2+textExtents.x_bearing,
                          fontExtents.ascent+textExtents.y_bearing,
                          text.c_str());
  }

  void MapPainterCairo::DrawSymbol(const SymbolStyle* style,
                                   double x, double y)
  {
    switch (style->GetStyle()) {
    case SymbolStyle::none:
      break;
    case SymbolStyle::box:
      cairo_new_path(draw);
      cairo_set_source_rgba(draw,
                            style->GetFillR(),
                            style->GetFillG(),
                            style->GetFillB(),
                            style->GetFillA());
      cairo_set_line_width(draw,1);

      cairo_rectangle(draw,
                      x-style->GetSize()/2,y-style->GetSize()/2,
                      style->GetSize(),style->GetSize());
      cairo_fill(draw);
      break;
    case SymbolStyle::circle:
      cairo_new_path(draw);
      cairo_set_source_rgba(draw,
                            style->GetFillR(),
                            style->GetFillG(),
                            style->GetFillB(),
                            style->GetFillA());
      cairo_set_line_width(draw,1);

      cairo_arc(draw,
                x,y,
                style->GetSize(),
                0,2*M_PI);
      cairo_fill(draw);
      break;
    case SymbolStyle::triangle:
      cairo_new_path(draw);
      cairo_set_source_rgba(draw,
                            style->GetFillR(),
                            style->GetFillG(),
                            style->GetFillB(),
                            style->GetFillA());
      cairo_set_line_width(draw,1);

      cairo_move_to(draw,x-style->GetSize()/2,y+style->GetSize()/2);
      cairo_line_to(draw,x,y-style->GetSize()/2);
      cairo_line_to(draw,x+style->GetSize()/2,y+style->GetSize()/2);
      cairo_line_to(draw,x-style->GetSize()/2,y+style->GetSize()/2);
      cairo_fill(draw);
      break;
    }
  }

  void MapPainterCairo::DrawIcon(const IconStyle* style,
                                 double x, double y)
  {
    assert(style->GetId()>0);
    assert(style->GetId()!=std::numeric_limits<size_t>::max());
    assert(style->GetId()<=images.size());
    assert(images[style->GetId()-1]!=NULL);

    cairo_set_source_surface(draw,images[style->GetId()-1],x-7,y-7);
    cairo_paint(draw);
  }

  void MapPainterCairo::DrawPath(const Projection& projection,
                                 const MapParameter& parameter,
                                 double r, double g, double b, double a,
                                 double width,
                                 const std::vector<double>& dash,
                                 CapStyle startCap,
                                 CapStyle endCap,
                                 const TransPolygon& path)
  {
    SetLineAttributes(r,g,b,a,width,dash);

    if (startCap==capRound &&
        endCap==capRound &&
        dash.empty()) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
    }
    else {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
    }

    for (size_t i=path.GetStart(); i<=path.GetEnd(); i++) {
      if (path.points[i].draw) {
        if (i==path.GetStart()) {
          cairo_new_path(draw);
          cairo_move_to(draw,
                        path.points[i].x,
                        path.points[i].y);
        }
        else {
          cairo_line_to(draw,
                        path.points[i].x,
                        path.points[i].y);
        }
      }
    }

    cairo_stroke(draw);

    if (dash.empty() &&
      startCap==capRound &&
      endCap!=capRound) {
      cairo_new_path(draw);
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_line_width(draw,width);

      cairo_move_to(draw,path.points[path.GetStart()].x,path.points[path.GetStart()].y);
      cairo_line_to(draw,path.points[path.GetStart()].x,path.points[path.GetStart()].y);
      cairo_stroke(draw);
    }

    if (dash.empty() &&
      endCap==capRound &&
      startCap!=capRound) {
      cairo_new_path(draw);
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_line_width(draw,width);

      cairo_move_to(draw,path.points[path.GetEnd()].x,path.points[path.GetEnd()].y);
      cairo_line_to(draw,path.points[path.GetEnd()].x,path.points[path.GetEnd()].y);
      cairo_stroke(draw);
    }
  }

  void MapPainterCairo::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 TypeId type,
                                 const FillStyle& fillStyle,
                                 const TransPolygon& area)
  {
    if (fillStyle.HasPattern() &&
        projection.GetMagnification()>=fillStyle.GetPatternMinMag() &&
        HasPattern(parameter,fillStyle)) {
      assert(fillStyle.GetPatternId()<=images.size());
      assert(images[fillStyle.GetPatternId()-1]!=NULL);

      cairo_set_source(draw,patterns[fillStyle.GetPatternId()-1]);
    }
    else {
      cairo_set_source_rgba(draw,
                            fillStyle.GetFillR(),
                            fillStyle.GetFillG(),
                            fillStyle.GetFillB(),
                            fillStyle.GetFillA());
      cairo_set_line_width(draw,1);
    }

    for (size_t i=area.GetStart(); i<=area.GetEnd(); i++) {
      if (area.points[i].draw) {
        if (i==area.GetStart()) {
          cairo_new_path(draw);
          cairo_move_to(draw,area.points[i].x,area.points[i].y);
        }
        else {
          cairo_line_to(draw,area.points[i].x,area.points[i].y);
        }
      }
    }

    cairo_line_to(draw,area.points[area.GetStart()].x,area.points[area.GetStart()].y);

    cairo_fill_preserve(draw);

    double borderWidth=GetProjectedWidth(projection, fillStyle.GetBorderMinPixel(), fillStyle.GetBorderWidth());

    if (borderWidth>0.0) {
      SetLineAttributes(fillStyle.GetBorderR(),
                        fillStyle.GetBorderG(),
                        fillStyle.GetBorderB(),
                        fillStyle.GetBorderA(),
                        borderWidth,
                        fillStyle.GetBorderDash());

      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);

      cairo_stroke(draw);
    }
  }

  void MapPainterCairo::DrawArea(const FillStyle& style,
                                 const MapParameter& parameter,
                                 double x,
                                 double y,
                                 double width,
                                 double height)
  {
    cairo_set_source_rgba(draw,
                          style.GetFillR(),
                          style.GetFillG(),
                          style.GetFillB(),
                          1);

    cairo_rectangle(draw,x,y,width,height);
    cairo_fill(draw);
  }

  bool MapPainterCairo::DrawMap(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data,
                                cairo_t *draw)
  {
    this->draw=draw;

    Draw(styleConfig,
         projection,
         parameter,
         data);

    return true;
  }
}

