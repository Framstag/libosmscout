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

#include <sys/time.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <list>
#include <cstdlib>

#include <osmscout/LoaderPNG.h>
#include <osmscout/Util.h>

namespace osmscout {

  /* Returns Euclidean distance between two points */
  static double two_points_distance(cairo_path_data_t *a, cairo_path_data_t *b)
  {
    double dx, dy;

    dx = b->point.x - a->point.x;
    dy = b->point.y - a->point.y;

    return sqrt (dx * dx + dy * dy);
  }

  typedef double parametrization_t;

  /* Compute parametrization info. That is, for each part of the
    * cairo path, tags it with its length.
  */
  static parametrization_t* parametrize_path(cairo_path_t *path)
  {
    int i;
    cairo_path_data_t *data, last_move_to, current_point;
    parametrization_t *parametrization;

    current_point.point.x=0;
    current_point.point.y=0;

    parametrization = (parametrization_t*)malloc (path->num_data * sizeof (parametrization[0]));

    for (i=0; i < path->num_data; i += path->data[i].header.length) {
      data = &path->data[i];
      parametrization[i] = 0.0;
      switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO:
        last_move_to = data[1];
        current_point = data[1];
        break;
      case CAIRO_PATH_CLOSE_PATH:
        /* Make it look like it's a line_to to last_move_to */
        data = (&last_move_to) - 1;
        /* fall through */
      case CAIRO_PATH_LINE_TO:
        parametrization[i] = two_points_distance (&current_point, &data[1]);
        current_point = data[1];
        break;
      case CAIRO_PATH_CURVE_TO:
        assert(false);
        // not with cairo_path_copy_flat!
        break;
      default:
        assert(false);
      }
    }

    return parametrization;
  }


  typedef void (*transform_point_func_t)(void *closure, double *x, double *y);

  /* Project a path using a function. Each point of the path (including
    * Bezier control points) is passed to the function for transformation.
  */
  static void transform_path(cairo_path_t *path, transform_point_func_t f, void *closure)
  {
    int i;
    cairo_path_data_t *data;

    for (i=0; i < path->num_data; i += path->data[i].header.length) {
      data = &path->data[i];
      switch (data->header.type) {
      case CAIRO_PATH_CURVE_TO:
        f (closure, &data[3].point.x, &data[3].point.y);
        f (closure, &data[2].point.x, &data[2].point.y);
      case CAIRO_PATH_MOVE_TO:
      case CAIRO_PATH_LINE_TO:
        f (closure, &data[1].point.x, &data[1].point.y);
        break;
      case CAIRO_PATH_CLOSE_PATH:
        break;
      default:
        assert(false);
      }
    }
  }


  /* Simple struct to hold a path and its parametrization */
  typedef struct {
    cairo_path_t *path;
    parametrization_t *parametrization;
  } parametrized_path_t;


  /* Project a point X,Y onto a parameterized path. The final point is
    * where you get if you walk on the path forward from the beginning for X
    * units, then stop there and walk another Y units perpendicular to the
    * path at that point. In more detail:
    *
    * There's three pieces of math involved:
    *
    * - The parametric form of the Line equation
    * http://en.wikipedia.org/wiki/Line
    *
    * - The parametric form of the Cubic BÃ©zier curve equation
    * http://en.wikipedia.org/wiki/B%C3%A9zier_curve
    *
    * - The Gradient (aka multi-dimensional derivative) of the above
    * http://en.wikipedia.org/wiki/Gradient
    *
    * The parametric forms are used to answer the question of "where will I be
    * if I walk a distance of X on this path". The Gradient is used to answer
    * the question of "where will I be if then I stop, rotate left for 90
    * degrees and walk straight for a distance of Y".
  */
  static void point_on_path(parametrized_path_t *param, double *x, double *y)
  {
    int i;
    double ratio, the_y = *y, the_x = *x, dx, dy;
    cairo_path_data_t *data, last_move_to, current_point;
    cairo_path_t *path = param->path;
    parametrization_t *parametrization = param->parametrization;

    current_point.point.x=0;
    current_point.point.y=0;

    for (i=0; i + path->data[i].header.length < path->num_data &&
         (the_x > parametrization[i] ||
          path->data[i].header.type == CAIRO_PATH_MOVE_TO);
          i += path->data[i].header.length) {
      the_x -= parametrization[i];
      data = &path->data[i];
      switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO:
        current_point = data[1];
        last_move_to = data[1];
        break;
      case CAIRO_PATH_LINE_TO:
        current_point = data[1];
        break;
      case CAIRO_PATH_CURVE_TO:
        assert(false);
        // not with cairo_path_copy_flat!
        break;
      case CAIRO_PATH_CLOSE_PATH:
        break;
      default:
        assert(false);
      }
    }
    data = &path->data[i];

    switch (data->header.type) {

    case CAIRO_PATH_MOVE_TO:
      break;
    case CAIRO_PATH_CLOSE_PATH:
      /* Make it look like it's a line_to to last_move_to */
      data = (&last_move_to) - 1;
      /* fall through */
    case CAIRO_PATH_LINE_TO:
      {
        ratio = the_x / parametrization[i];
        /* Line polynomial */
        *x = current_point.point.x * (1 - ratio) + data[1].point.x * ratio;
        *y = current_point.point.y * (1 - ratio) + data[1].point.y * ratio;

        /* Line gradient */
        dx = -(current_point.point.x - data[1].point.x);
        dy = -(current_point.point.y - data[1].point.y);

        /*optimization for: ratio = the_y / sqrt (dx * dx + dy * dy);*/
        ratio = the_y / parametrization[i];
        *x += -dy * ratio;
        *y += dx * ratio;
      }
      break;
    case CAIRO_PATH_CURVE_TO:
        assert(false);
        // not with cairo_path_copy_flat!
      break;
    default:
      assert(false);
    }
  }

  /* Projects the current path of cr onto the provided path. */
  static void map_path_onto(cairo_t *cr, cairo_path_t *path)
  {
    cairo_path_t        *current_path;
    parametrized_path_t param;

    param.path=path;
    param.parametrization=parametrize_path(path);

    current_path=cairo_copy_path(cr);
    cairo_new_path(cr);

    transform_path(current_path,
                   (transform_point_func_t) point_on_path,
                   &param);

    cairo_append_path(cr,current_path);

    cairo_path_destroy(current_path);
    free(param.parametrization);
  }


  typedef void (*draw_path_func_t) (cairo_t *cr);

  static void draw_twisted(cairo_t *cr, double x, double y, const char *text)
  {
    cairo_path_t *path;

    cairo_save(cr);

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
    path=cairo_copy_path_flat(cr);
    //path=cairo_copy_path(cr);

    cairo_new_path(cr);

    cairo_move_to(cr,x,y);
    cairo_text_path(cr,text);
    map_path_onto(cr,path);
    cairo_fill(cr);

    cairo_path_destroy(path);

    cairo_restore(cr);
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

  bool MapPainterCairo::HasPattern(const StyleConfig& styleConfig,
                                   const MapParameter& parameter,
                                   PatternStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+style.GetPatternName()+".png";

      cairo_surface_t *image=osmscout::LoadPNG(filename);

      if (image!=NULL) {
        images.resize(images.size()+1,image);
        style.SetId(images.size());
        patterns.resize(images.size(),NULL);

        patterns[patterns.size()-1]=cairo_pattern_create_for_surface(images[images.size()-1]);
        cairo_pattern_set_extend(patterns[patterns.size()-1],CAIRO_EXTEND_REPEAT);

        cairo_matrix_t matrix;

        cairo_matrix_init_scale(&matrix,1,1);
        cairo_pattern_set_matrix(patterns[patterns.size()-1],&matrix);

        std::cout << "Loaded image " << filename << " => id " << style.GetId() << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading icon file '" << style.GetPatternName() << "'" << std::endl;
    style.SetId(std::numeric_limits<size_t>::max());

    return false;
  }

  cairo_scaled_font_t* MapPainterCairo::GetScaledFont(const MapParameter& parameter,
                                                      size_t fontSize)
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
                                         const std::vector<TransPoint>& nodes)
  {
    assert(style.GetStyle()==LabelStyle::contour);

    cairo_scaled_font_t *font=GetScaledFont(parameter,
                                            style.GetSize());

    cairo_set_scaled_font(draw,font);

    double length=0;
    double xo=0;
    double yo=0;

    cairo_new_path(draw);

    if (nodes[0].x<nodes[nodes.size()-1].x) {
      bool start=true;

      for (size_t j=0; j<nodes.size(); j++) {
        if (nodes[j].draw) {
          if (start) {
            cairo_move_to(draw,nodes[j].x,
                          nodes[j].y);
            start=false;
          }
          else {
            cairo_line_to(draw,nodes[j].x,
                          nodes[j].y);
            length+=sqrt(pow(nodes[j].x-xo,2)+
                         pow(nodes[j].y-yo,2));
          }

          xo=nodes[j].x;
          yo=nodes[j].y;
        }
      }
    }
    else {
      bool start=true;

      for (size_t j=0; j<nodes.size(); j++) {
        size_t idx=nodes.size()-j-1;

        if (nodes[idx].draw) {
          if (start) {
            cairo_move_to(draw,
                          nodes[idx].x,
                          nodes[idx].y);
            start=false;
          }
          else {
            cairo_line_to(draw,
                          nodes[idx].x,
                          nodes[idx].y);
            length+=sqrt(pow(nodes[idx].x-xo,2)+
                         pow(nodes[idx].y-yo,2));
          }

          xo=nodes[idx].x;
          yo=nodes[idx].y;
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

    draw_twisted(draw,
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
                                 double r,
                                 double g,
                                 double b,
                                 double a,
                                 double width,
                                 const std::vector<double>& dash,
                                 CapStyle startCap,
                                 CapStyle endCap,
                                 const std::vector<TransPoint>& nodes)
  {
    double dashArray[10];

    assert(dash.size()<=10);

    cairo_set_source_rgba(draw,r,g,b,a);

    cairo_set_line_width(draw,width);

    if (startCap==capRound &&
        endCap==capRound &&
        dash.size()==0) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
    }
    else {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
    }

    if (dash.size()==0) {
      cairo_set_dash(draw,NULL,0,0);
    }
    else {
      for (size_t i=0; i<dash.size(); i++) {
        dashArray[i]=dash[i]*width;
      }
      cairo_set_dash(draw,dashArray,dash.size(),0);
    }

    size_t firstNode=0;
    size_t lastNode=0;
    bool   start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (nodes[i].draw) {
        if (start) {
          cairo_new_path(draw);
          cairo_move_to(draw,nodes[i].x,nodes[i].y);
          start=false;
          firstNode=i;
          lastNode=i;
        }
        else {
          cairo_line_to(draw,nodes[i].x,nodes[i].y);
          lastNode=i;
        }

        //nodesDrawnCount++;
      }
      else {
        //nodesOutCount++;
      }
    }
    //nodesAllCount+=way->nodes.size();

    cairo_stroke(draw);

    if (dash.size()==0 &&
      startCap==capRound &&
      endCap!=capRound) {
      cairo_new_path(draw);
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_line_width(draw,width);

      cairo_move_to(draw,nodes[firstNode].x,nodes[firstNode].y);
      cairo_line_to(draw,nodes[firstNode].x,nodes[firstNode].y);
      cairo_stroke(draw);
    }

    if (dash.size()==0 &&
      endCap==capRound &&
      startCap!=capRound) {
      cairo_new_path(draw);
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_line_width(draw,width);

      cairo_move_to(draw,nodes[lastNode].x,nodes[lastNode].y);
      cairo_line_to(draw,nodes[lastNode].x,nodes[lastNode].y);
      cairo_stroke(draw);
    }
  }

  void MapPainterCairo::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 TypeId type,
                                 const FillStyle& fillStyle,
                                 const LineStyle* lineStyle,
                                 const std::vector<TransPoint>& nodes)
  {
    cairo_set_source_rgba(draw,
                          fillStyle.GetFillR(),
                          fillStyle.GetFillG(),
                          fillStyle.GetFillB(),
                          1);
    cairo_set_line_width(draw,1);

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (nodes[i].draw) {
        if (start) {
          cairo_new_path(draw);
          cairo_move_to(draw,nodes[i].x,nodes[i].y);
          start=false;
        }
        else {
          cairo_line_to(draw,nodes[i].x,nodes[i].y);
        }
        //nodesDrawnCount++;
      }

      //nodesAllCount++;
    }

    cairo_fill(draw);

    if (lineStyle!=NULL) {
      DrawPath(projection,
               parameter,
               lineStyle->GetLineR(),
               lineStyle->GetLineG(),
               lineStyle->GetLineB(),
               lineStyle->GetLineA(),
               borderWidth[(size_t)type],
               lineStyle->GetDash(),
               capRound,
               capRound,
               nodes);
    }
  }

  void MapPainterCairo::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 TypeId type,
                                 const PatternStyle& patternStyle,
                                 const LineStyle* lineStyle,
                                 const std::vector<TransPoint>& nodes)
  {
    assert(patternStyle.GetId()>0);
    assert(patternStyle.GetId()!=std::numeric_limits<size_t>::max());
    assert(patternStyle.GetId()<=images.size());
    assert(images[patternStyle.GetId()-1]!=NULL);

    cairo_set_source(draw,patterns[patternStyle.GetId()-1]);

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (nodes[i].draw) {
        if (start) {
          cairo_new_path(draw);
          cairo_move_to(draw,nodes[i].x,nodes[i].y);
          start=false;
        }
        else {
          cairo_line_to(draw,nodes[i].x,nodes[i].y);
        }
        //nodesDrawnCount++;
      }

      //nodesAllCount++;
    }

    cairo_fill(draw);

    if (lineStyle!=NULL) {
      DrawPath(projection,
               parameter,
               lineStyle->GetLineR(),
               lineStyle->GetLineG(),
               lineStyle->GetLineB(),
               lineStyle->GetLineA(),
               borderWidth[(size_t)type],
               lineStyle->GetDash(),
               capRound,
               capRound,
               nodes);
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

