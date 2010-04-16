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

#include <osmscout/MapPainter.h>

#include <sys/time.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <list>
#include <cstdlib>

#include <osmscout/LoaderPNG.h>

static size_t optimizeLimit=512;
static double relevantPosDeriviation=2.0;
static double relevantSlopeDeriviation=0.1;

namespace osmscout {

  static const double gradtorad=2*M_PI/360;

  static double longDash[]= {7,3};
  static double dotted[]= {1,2};
  static double lineDot[]= {7,3,1,3};

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
    cairo_path_t *current_path;
    parametrized_path_t param;

    param.path = path;
    param.parametrization = parametrize_path (path);

    current_path = cairo_copy_path (cr);
    cairo_new_path (cr);

    transform_path (current_path,
                    (transform_point_func_t) point_on_path, &param);

    cairo_append_path (cr, current_path);
  }


  typedef void (*draw_path_func_t) (cairo_t *cr);

  static void draw_twisted(cairo_t *cr, double x, double y, const char *text)
  {
    cairo_path_t *path;

    cairo_save (cr);

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
    path = cairo_copy_path_flat (cr);
    //path = cairo_copy_path (cr);

    cairo_new_path (cr);

    cairo_move_to(cr,x,y);
    cairo_text_path (cr, text);
    map_path_onto (cr, path);
    cairo_fill (cr);

    cairo_restore (cr);
  }

  MapPainter::MapPainter(const Database& database)
   : database(database)
  {
    drawNode.resize(10000); // TODO: Calculate matching size
    outNode.resize(10000); // TODO: Calculate matching size
    nodeX.resize(10000);
    nodeY.resize(10000);
  }

  MapPainter::~MapPainter()
  {
    // no code
  }

  bool MapPainter::IsVisible(const std::vector<Point>& nodes) const
  {
    if (nodes.size()==0) {
      return false;
    }

    // Bounding box
    double lonMin=nodes[0].lon;
    double lonMax=nodes[0].lon;
    double latMin=nodes[0].lat;
    double latMax=nodes[0].lat;

    for (size_t i=1; i<nodes.size(); i++) {
      lonMin=std::min(lonMin,nodes[i].lon);
      lonMax=std::max(lonMax,nodes[i].lon);
      latMin=std::min(latMin,nodes[i].lat);
      latMax=std::max(latMax,nodes[i].lat);
    }

    // If bounding box is neither left or right nor above or below
    // it must somehow cover the map area.
    return !(lonMin>this->lonMax ||
             lonMax<this->lonMin ||
             latMin>this->latMax ||
             latMax<this->latMin);
  }

  void MapPainter::TransformArea(const std::vector<Point>& nodes)
  {
    if (magnification>optimizeLimit) {
      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
        TransformGeoToPixel(nodes[i].lon,nodes[i].lat,
                            nodeX[i],nodeY[i]);
      }
    }
    else {
      drawNode[0]=true;
      drawNode[nodes.size()-1]=true;

      // Drop every point that is on direct line between two points A and B
      for (size_t i=1; i+1<nodes.size(); i++) {
        drawNode[i]=std::abs((nodes[i].lon-nodes[i-1].lon)/
                             (nodes[i].lat-nodes[i-1].lat)-
                             (nodes[i+1].lon-nodes[i].lon)/
                             (nodes[i+1].lat-nodes[i].lat))>=relevantSlopeDeriviation;
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (drawNode[i]) {
          TransformGeoToPixel(nodes[i].lon,nodes[i].lat,
                              nodeX[i],nodeY[i]);
        }
      }

      // Drop all points that do not differ in position from the previous node
      for (size_t i=1; i<nodes.size()-1; i++) {
        if (drawNode[i]) {
          size_t j=i+1;
          while (!drawNode[j]) {
            j++;
          }

          if (std::fabs(nodeX[j]-nodeX[i])<=relevantPosDeriviation &&
              std::fabs(nodeY[j]-nodeY[i])<=relevantPosDeriviation) {
            drawNode[i]=false;
          }
        }
      }
    }
  }

  void MapPainter::TransformWay(const std::vector<Point>& nodes)
  {
    if (magnification>optimizeLimit) {
      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
        TransformGeoToPixel(nodes[i].lon,nodes[i].lat,
                            nodeX[i],nodeY[i]);
      }
    }
    else {
      size_t a;

      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
      }

      if (nodes.size()>=3) {
        a=0;
        while (a+1<nodes.size()) {
          if (nodes[a].lon>=lonMin && nodes[a].lon<=lonMax &&
              nodes[a].lat>=latMin && nodes[a].lat<=latMax) {
            break;
          }

          a++;
        }

        if (a>1) {
          for (size_t i=0; i<a-1; i++) {
            drawNode[i]=false;
          }
        }
      }

      if (nodes.size()>=3) {
        a=nodes.size()-1;
        while (a>0) {
          if (nodes[a].lon>=lonMin && nodes[a].lon<=lonMax &&
              nodes[a].lat>=latMin && nodes[a].lat<=latMax) {
            break;
          }

          a--;
        }

        if (a<nodes.size()-2) {
          for (size_t i=a+2; i<nodes.size(); i++) {
            drawNode[i]=false;
          }
        }
      }

      // Drop every point that is on direct line between two points A and B
      for (size_t i=0; i+2<nodes.size(); i++) {
        if (drawNode[i]) {
          size_t j=i+1;
          while (j<nodes.size() && !drawNode[j]) {
            j++;
          }

          size_t k=j+1;
          while (k<nodes.size() && !drawNode[k]) {
            k++;
          }

          if (j<nodes.size() && k<nodes.size()) {
            drawNode[j]=std::abs((nodes[j].lon-nodes[i].lon)/
                               (nodes[j].lat-nodes[i].lat)-
                               (nodes[k].lon-nodes[j].lon)/
                               (nodes[k].lat-nodes[j].lat))>=relevantSlopeDeriviation;
          }
        }
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (drawNode[i]) {
          TransformGeoToPixel(nodes[i].lon,nodes[i].lat,
                              nodeX[i],nodeY[i]);
        }
      }

      // Drop all points that do not differ in position from the previous node
      if (nodes.size()>2) {
        for (size_t i=1; i<nodes.size()-1; i++) {
          if (drawNode[i]) {
            size_t j=i+1;

            while (j+1<nodes.size() &&
                   !drawNode[j]) {
              j++;
            }

            if (std::fabs(nodeX[j]-nodeX[i])<=relevantPosDeriviation &&
                std::fabs(nodeY[j]-nodeY[i])<=relevantPosDeriviation) {
              drawNode[i]=false;
            }
          }
        }
      }

      /*
      // Check which nodes or not visible in the given way
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (way->nodes[i].lon<lonMin || way->nodes[i].lon>lonMax ||
            way->nodes[i].lat<latMin || way->nodes[i].lat>latMax){
          outNode[i]=true;
        }
      }

      if (outNode[1]) {
        drawNode[0]=false;
      }

      for (size_t i=1; i<way->nodes.size()-1; i++) {
        if (outNode[i-1] && outNode[i+1]) {
          drawNode[i]=false;
        }
      }

      if (outNode[way->nodes.size()-2]) {
        drawNode[way->nodes.size()-1]=false;
      }*/
    }
  }

  bool MapPainter::GetCenterPixel(const std::vector<Point>& nodes,
                                  double& cx,
                                  double& cy)
  {
    if (nodes.size()==0) {
      return false;
    }

    double xmin=nodes[0].lon;
    double xmax=nodes[0].lon;
    double ymin=nodes[0].lat;
    double ymax=nodes[0].lat;

    for (size_t j=1; j<nodes.size(); j++) {
      xmin=std::min(xmin,nodes[j].lon);
      xmax=std::max(xmax,nodes[j].lon);
      ymin=std::min(ymin,nodes[j].lat);
      ymax=std::max(ymax,nodes[j].lat);
    }

    TransformGeoToPixel(xmin,ymin,xmin,ymin);
    TransformGeoToPixel(xmax,ymax,xmax,ymax);

    cx=xmin+(xmax-xmin)/2;
    cy=ymin+(ymax-ymin)/2;

    return true;
  }

  bool MapPainter::CheckImage(const StyleConfig& styleConfig,
                              IconStyle::Icon icon)
  {
    if (imageChecked.size()<=icon) {
      imageChecked.resize(icon+1,false);
      image.resize(icon+1,NULL);
    }

    if (imageChecked[icon]) {
      return image[icon]!=NULL;
    }

    std::string filename=std::string("../libosmscout/data/icons/14x14/standard/")+
                         styleConfig.GetIconNameByIcon(icon)+".png";

    image[icon]=osmscout::LoadPNG(filename);

    if (image[icon]==NULL) {
      std::cerr << "ERROR while loading icon file '" << filename << "'" << std::endl;
    }

    imageChecked[icon]=true;

    return false;
  }

  bool MapPainter::TransformPixelToGeo(double x, double y,
                                       double& lon, double& lat)
  {
    lon=lonMin+(lonMax-lonMin)*x/width;
    lat=latMin+(latMax-latMin)*y/height;

    return true;
  }

  bool MapPainter::TransformGeoToPixel(double lon, double lat,
                                       double& x, double& y)
  {
    x=(lon*gradtorad-hmin)*hscale;
    y=height-(atanh(sin(lat*gradtorad))-vmin)*vscale;

    return true;
  }

  cairo_scaled_font_t* MapPainter::GetScaledFont(cairo_t* draw,
                                                 size_t fontSize)
  {
    std::map<size_t,cairo_scaled_font_t*>::const_iterator f;

    f=font.find(fontSize);

    if (f==font.end()) {
      cairo_matrix_t       scaleMatrix;
      cairo_matrix_t       transformMatrix;
      cairo_font_options_t *options;
      cairo_scaled_font_t  *scaledFont;

      cairo_matrix_init_scale(&scaleMatrix,fontSize,fontSize);
      cairo_get_matrix(draw,&transformMatrix);
      options=cairo_font_options_create();
      cairo_font_options_set_hint_style (options,CAIRO_HINT_STYLE_NONE);
      cairo_font_options_set_hint_metrics (options,CAIRO_HINT_METRICS_OFF);

      cairo_select_font_face(draw,
                             "sans-serif",
                             CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);

      scaledFont=cairo_scaled_font_create(cairo_get_font_face(draw),
                                          &scaleMatrix,
                                          &transformMatrix,
                                          options);

      cairo_font_options_destroy(options);

      return font.insert(std::pair<size_t,cairo_scaled_font_t*>(fontSize,scaledFont)).first->second;
    }
    else {
      return f->second;
    }
  }

  void MapPainter::DrawLabel(cairo_t* draw,
                             double magnification,
                             const LabelStyle& style,
                             const std::string& text,
                             double x, double y)
  {
    // TODO: If the point is offscreen move it into the screen...

    if (style.GetStyle()==LabelStyle::normal) {
      cairo_scaled_font_t *font;

      cairo_font_extents_t fontExtents;
      cairo_text_extents_t textExtents;

      double fontSize=style.GetSize()*9.0;
      double r=style.GetTextR();
      double g=style.GetTextG();
      double b=style.GetTextB();
      double a=style.GetTextA();

      if (magnification>style.GetScaleAndFadeMag()) {
        double factor=log2(magnification)-log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        a=a/factor;
      }

      font=GetScaledFont(draw,fontSize);

      cairo_set_scaled_font(draw,font);

      cairo_scaled_font_extents(font,&fontExtents);
      cairo_scaled_font_text_extents(font,text.c_str(),&textExtents);

      cairo_set_source_rgba(draw,r,g,b,a);

      if (x-textExtents.width/2+textExtents.x_bearing>=width ||
          x+textExtents.width/2+textExtents.x_bearing<0 ||
          y-textExtents.height/2+textExtents.x_bearing>=height ||
          y+textExtents.width/2+textExtents.x_bearing<0) {
        return;
      }

      cairo_move_to(draw,x-textExtents.width/2+textExtents.x_bearing,
                    y-textExtents.height/2-textExtents.y_bearing);
      cairo_show_text(draw,text.c_str());
      cairo_stroke(draw);
    }
    else if (style.GetStyle()==LabelStyle::plate) {
      static const double outerWidth = 4;
      static const double innerWidth = 2;

      cairo_scaled_font_t *font;

      font=GetScaledFont(draw,style.GetSize()*9.0);

      cairo_set_scaled_font(draw,font);

      cairo_font_extents_t fontExtents;
      cairo_text_extents_t textExtents;

      cairo_scaled_font_extents(font,&fontExtents);
      cairo_scaled_font_text_extents(font,text.c_str(),&textExtents);

      if (x-textExtents.width/2+textExtents.x_bearing-outerWidth>=width ||
          x+textExtents.width/2+textExtents.x_bearing-outerWidth<0 ||
          y-textExtents.height/2+textExtents.x_bearing+outerWidth>=height ||
          y+textExtents.width/2+textExtents.x_bearing+outerWidth<0) {
        return;
      }

      cairo_set_line_width(draw,1);

      cairo_set_source_rgba(draw,
                            style.GetBgR(),
                            style.GetBgG(),
                            style.GetBgB(),
                            style.GetBgA());

      cairo_rectangle(draw,
                      x-textExtents.width/2+textExtents.x_bearing-outerWidth,
                      y-fontExtents.height/2-outerWidth,
                      textExtents.width+2*outerWidth,
                      fontExtents.height+2*outerWidth);
      cairo_fill(draw);

      cairo_set_source_rgba(draw,
                            style.GetBorderR(),
                            style.GetBorderG(),
                            style.GetBorderB(),
                            style.GetBorderA());

      cairo_rectangle(draw,
                      x-textExtents.width/2+textExtents.x_bearing-innerWidth,
                      y-fontExtents.height/2-innerWidth,
                      textExtents.width+2*innerWidth,
                      fontExtents.height+2*innerWidth);
      cairo_stroke(draw);

      cairo_set_source_rgba(draw,
                            style.GetTextR(),
                            style.GetTextG(),
                            style.GetTextB(),
                            style.GetTextA());

      cairo_move_to(draw,x-textExtents.width/2+textExtents.x_bearing,
                    y-fontExtents.height/2+fontExtents.ascent);
      cairo_show_text(draw,text.c_str());
      cairo_stroke(draw);
    }
    else if (style.GetStyle()==LabelStyle::emphasize) {
      cairo_font_extents_t fontExtents;
      cairo_text_extents_t textExtents;

      cairo_save(draw);

      double fontSize=style.GetSize()*9.0;
      double r=style.GetTextR();
      double g=style.GetTextG();
      double b=style.GetTextB();
      double a=style.GetTextA();

      if (magnification>style.GetScaleAndFadeMag()) {
        double factor=log2(magnification)-log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        a=a/factor;
      }

      cairo_scaled_font_t *font;

      font=GetScaledFont(draw,fontSize);

      cairo_set_scaled_font(draw,font);

      cairo_scaled_font_extents(font,&fontExtents);
      cairo_scaled_font_text_extents(font,text.c_str(),&textExtents);

      if (x-textExtents.width/2+textExtents.x_bearing>=width ||
          x+textExtents.width/2+textExtents.x_bearing<0 ||
          y-textExtents.height/2+textExtents.x_bearing>=height ||
          y+textExtents.width/2+textExtents.x_bearing<0) {
        return;
      }

      cairo_move_to(draw,x-textExtents.width/2+textExtents.x_bearing,
                    y-textExtents.height/2-textExtents.y_bearing);

      cairo_text_path(draw,text.c_str());
      cairo_set_source_rgba(draw,1,1,1,a);
      cairo_set_line_width(draw,2.0);
      cairo_stroke_preserve(draw);
      cairo_set_source_rgba(draw,r,g,b,a);
      cairo_fill(draw);

      cairo_restore(draw);
    }
  }

  void MapPainter::DrawContourLabel(cairo_t* draw,
                                    const LabelStyle& style,
                                    const std::string& text,
                                    const std::vector<Point>& nodes)
  {
    cairo_scaled_font_t *font;

    font=GetScaledFont(draw,style.GetSize()*9.0);

    cairo_set_scaled_font(draw,font);

    cairo_new_path(draw);
    double length=0;
    double xo=0,yo=0;
    double x=0,y=0;
    if (nodes[0].lon<nodes[nodes.size()-1].lon) {
      for (size_t j=0; j<nodes.size(); j++) {
        xo=x;
        yo=y;

        TransformGeoToPixel(nodes[j].lon,nodes[j].lat,
                            x,y);
        if (j==0) {
          cairo_move_to(draw,x,y);
        }
        else {
          cairo_line_to(draw,x,y);
          length+=sqrt(pow(x-xo,2)+pow(y-yo,2));
        }
      }
    }
    else {
      for (size_t j=0; j<nodes.size(); j++) {
        xo=x;
        yo=y;

        TransformGeoToPixel(nodes[nodes.size()-j-1].lon,nodes[nodes.size()-j-1].lat,
                            x,y);

        if (j==0) {
          cairo_move_to(draw,x,y);
        }
        else {
          cairo_line_to(draw,x,y);
          length+=sqrt(pow(x-xo,2)+pow(y-yo,2));
        }
      }
    }

    cairo_text_extents_t textExtents;

    cairo_scaled_font_text_extents(font,text.c_str(),&textExtents);

    if (length>=textExtents.width) {
      cairo_font_extents_t fontExtents;

      cairo_scaled_font_extents(font,&fontExtents);

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
  }

  void MapPainter::DrawSymbol(cairo_t* draw,
                              const SymbolStyle* style,
                              double x, double y)
  {
    switch (style->GetStyle()) {
    case SymbolStyle::none:
      break;
    case SymbolStyle::box:
      cairo_set_source_rgba(draw,
                            style->GetFillR(),
                            style->GetFillG(),
                            style->GetFillB(),
                            style->GetFillA());
      cairo_set_line_width(draw,1);

      cairo_new_path(draw);
      cairo_rectangle(draw,
                      x-style->GetSize()/2,y-style->GetSize()/2,
                      style->GetSize(),style->GetSize());
      cairo_fill(draw);
      break;
    case SymbolStyle::circle:
      std::cout << "Draw circle symbol" << std::endl;
      cairo_set_source_rgba(draw,
                            style->GetFillR(),
                            style->GetFillG(),
                            style->GetFillB(),
                            style->GetFillA());
      cairo_set_line_width(draw,1);

      cairo_new_path(draw);
      cairo_arc(draw,
                x,y,
                style->GetSize(),
                0,2*M_PI);
      cairo_fill(draw);
      break;
    case SymbolStyle::triangle:
      cairo_set_source_rgba(draw,
                            style->GetFillR(),
                            style->GetFillG(),
                            style->GetFillB(),
                            style->GetFillA());
      cairo_set_line_width(draw,1);

      cairo_new_path(draw);
      cairo_move_to(draw,x-style->GetSize()/2,y+style->GetSize()/2);
      cairo_line_to(draw,x,y-style->GetSize()/2);
      cairo_line_to(draw,x+style->GetSize()/2,y+style->GetSize()/2);
      cairo_line_to(draw,x-style->GetSize()/2,y+style->GetSize()/2);
      cairo_fill(draw);
      break;
    }
  }

  void MapPainter::DrawIcon(cairo_t* draw,
                            const IconStyle* style,
                            double x, double y)
  {
    assert(image[style->GetIcon()]!=NULL);

    cairo_set_source_surface(draw,image[style->GetIcon()],x-7,y-7);
    cairo_paint(draw);
  }

  void MapPainter::DrawPath(const std::vector<Point>& nodes,
                            const LineStyle& style,
                            double width)
  {
    cairo_set_source_rgba(draw,
                          style.GetLineR(),
                          style.GetLineG(),
                          style.GetLineB(),
                          style.GetLineA());

    cairo_set_line_width(draw,width);

    switch (style.GetStyle()) {
    case LineStyle::none:
      // way should not be visible in this case!
      assert(false);
      break;
    case LineStyle::normal:
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      break;
    case LineStyle::longDash:
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
      cairo_set_dash(draw,longDash,2,0);
      break;
    case LineStyle::dotted:
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
      cairo_set_dash(draw,dotted,2,0);
      break;
    case LineStyle::lineDot:
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
      cairo_set_dash(draw,lineDot,4,0);
      break;
    }

    TransformWay(nodes);

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          cairo_move_to(draw,nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          cairo_line_to(draw,nodeX[i],nodeY[i]);
        }

        //nodesDrawnCount++;
      }
      else {
        //nodesOutCount++;
      }
    }
    //nodesAllCount+=way->nodes.size();

    cairo_stroke(draw);
  }

  void MapPainter::FillRegion(const std::vector<Point>& nodes,
                              const FillStyle& style)
  {
    cairo_set_source_rgb(draw,
                         style.GetFillR(),
                         style.GetFillG(),
                         style.GetFillB());
    cairo_set_line_width(draw,1);

    TransformArea(nodes);

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          cairo_move_to(draw,nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          cairo_line_to(draw,nodeX[i],nodeY[i]);
        }
        //nodesDrawnCount++;
      }

      //nodesAllCount++;
    }

    cairo_fill(draw);
  }

  void MapPainter::GetDimensions(double lon, double lat,
                                 double magnification,
                                 size_t width, size_t height,
                                 double& lonMin, double& latMin,
                                 double& lonMax, double& latMax)
  {
    double boxWidth,boxHeight;

    boxWidth=360/magnification;
    boxHeight=boxWidth*height/width;

    lonMin=lon-boxWidth/2;
    lonMax=lon+boxWidth/2;

    latMin=atan(sinh(atanh(sin(lat*gradtorad))-boxHeight/2*gradtorad))/gradtorad;
    latMax=atan(sinh(atanh(sin(lat*gradtorad))+boxHeight/2*gradtorad))/gradtorad;
  }

  void MapPainter::DrawWayOutline(const StyleConfig& styleConfig,
                                  TypeId type,
                                  bool isBridge,
                                  bool isTunnel,
                                  bool startIsJoint,
                                  bool endIsJoint,
                                  const std::vector<Point>& nodes)
  {
    if (!outline[(size_t)type] &&
        !(isBridge && magnification>=magCity) &&
        !(isTunnel && magnification>=magCity)) {
      return;
    }

    const LineStyle *style=styleConfig.GetWayLineStyle(type);

    if (style==NULL) {
      return;
    }

    if (!IsVisible(nodes)) {
      return;
    }

    if (isBridge && magnification>=magCity) {
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_source_rgb(draw,0.0,0.0,0.0);
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
    }
    else if (isTunnel && magnification>=magCity) {
      double tunnel[2];

      tunnel[0]=7+lineWidth[(size_t)type]+2*style->GetOutline();
      tunnel[1]=7+lineWidth[(size_t)type]+2*style->GetOutline();

      cairo_set_dash(draw,tunnel,2,0);
      if (magnification>=10000) {
        cairo_set_source_rgb(draw,0.75,0.75,0.75);
      }
      else {
        cairo_set_source_rgb(draw,0.5,0.5,0.5);
      }
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
    }
    else {
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_source_rgba(draw,
                            style->GetOutlineR(),
                            style->GetOutlineG(),
                            style->GetOutlineB(),
                            style->GetOutlineA());
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
    }

    cairo_set_line_width(draw,lineWidth[(size_t)type]+2*style->GetOutline());

    TransformWay(nodes);

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          cairo_move_to(draw,nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          cairo_line_to(draw,nodeX[i],nodeY[i]);
        }

        //nodesDrawnCount++;
      }
      else {
        //nodesOutCount++;
      }
    }
    //nodesAllCount+=way->nodes.size();

    cairo_stroke(draw);

    if (!startIsJoint) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_source_rgba(draw,
                            style->GetOutlineR(),
                            style->GetOutlineG(),
                            style->GetOutlineB(),
                            style->GetOutlineA());
      cairo_set_line_width(draw,lineWidth[(size_t)type]+2*style->GetOutline());

      cairo_move_to(draw,nodeX[0],nodeY[0]);
      cairo_line_to(draw,nodeX[0],nodeY[0]);
      cairo_stroke(draw);
    }

    if (!endIsJoint) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_source_rgba(draw,
                            style->GetOutlineR(),
                            style->GetOutlineG(),
                            style->GetOutlineB(),
                            style->GetOutlineA());
      cairo_set_line_width(draw,lineWidth[(size_t)type]+2*style->GetOutline());

      cairo_move_to(draw,nodeX[nodes.size()-1],nodeY[nodes.size()-1]);
      cairo_line_to(draw,nodeX[nodes.size()-1],nodeY[nodes.size()-1]);
      cairo_stroke(draw);
    }
  }

  void MapPainter::DrawWay(const StyleConfig& styleConfig,
                           TypeId type,
                           bool isBridge,
                           bool isTunnel,
                           const std::vector<Point>& nodes)
  {
    const LineStyle *style=styleConfig.GetWayLineStyle(type);

    if (style==NULL) {
      return;
    }

    if (style->GetLineA()==0.0) {
      return;
    }

    if (!IsVisible(nodes)) {
      return;
    }

    DrawPath(nodes,*style,lineWidth[(size_t)type]);

    waysDrawnCount++;
  }

  void MapPainter::DrawArea(const StyleConfig& styleConfig,
                            TypeId type,
                            size_t layer,
                            bool isBuilding,
                            const std::vector<Point>& nodes)
  {
    const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(type,isBuilding);

    if (fillStyle==NULL || fillStyle->GetLayer()!=layer) {
      return;
    }

    if (!IsVisible(nodes)) {
      return;
    }

    FillRegion(nodes,*fillStyle);

    areasDrawnCount++;

  }

  void MapPainter::DrawAreas(const StyleConfig& styleConfig)
  {
    for (size_t l=0; l<11; l++) {
      int layer=l-5;

      if (areaLayers[l]) {
        for (std::vector<Way>::const_iterator area=areas.begin();
             area!=areas.end();
             ++area) {
          DrawArea(styleConfig,
                   area->type,
                   layer,
                   area->flags & Way::isBuilding,
                   area->nodes);

          const LineStyle *lineStyle=styleConfig.GetAreaBorderStyle(area->type);

          if (lineStyle==NULL) {
            continue;
          }

          DrawPath(area->nodes,*lineStyle,borderWidth[(size_t)area->type]);
        }
      }

      if (relationAreaLayers[l]) {
        for (std::vector<Relation>::const_iterator relation=relationAreas.begin();
             relation!=relationAreas.end();
             ++relation) {
          const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(relation->type,
                                                                  false/*area->flags & Way::isBuilding*/);

          if (fillStyle==NULL ||
              fillStyle->GetLayer()!=layer) {
            continue;
          }

          //std::cout << "Drawing area relation " << relation->id << std::endl;

          bool drawn=false;
          for (size_t m=0; m<relation->roles.size(); m++) {
            if (relation->roles[m].role=="0") {
              drawn=true;

              if (!IsVisible(relation->roles[m].nodes)) {
                continue;
              }

              FillRegion(relation->roles[m].nodes,*fillStyle);
            }
          }

          if (!drawn) {
            std::cout << " Something is wrong with area relation " << relation->id << std::endl;
          }
        }
      }
    }
  }

  void MapPainter::DrawWays(const StyleConfig& styleConfig)
  {
    for (size_t l=0; l<11; l++) {
      int8_t layer=l-5;
      // Potential path outline

      if (wayLayers[l]) {
        for (std::vector<Way>::const_iterator way=ways.begin();
             way!=ways.end();
             ++way) {

          if (way->layer!=layer) {
            continue;
          }

          DrawWayOutline(styleConfig,
                         way->type,
                         way->flags & Way::isBridge,
                         way->flags & Way::isTunnel,
                         way->flags & Way::startIsJoint,
                         way->flags & Way::endIsJoint,
                         way->nodes);
        }

        for (std::vector<Way>::const_iterator way=ways.begin();
             way!=ways.end();
             ++way) {

          if (way->layer!=layer) {
            continue;
          }

          DrawWay(styleConfig,
                         way->type,
                         way->flags & Way::isBridge,
                         way->flags & Way::isTunnel,
                         way->nodes);
        }
      }

      if (relationWayLayers[l]) {
        for (std::vector<Relation>::const_iterator relation=relationWays.begin();
             relation!=relationWays.end();
             ++relation) {
          if (relation->layer!=layer) {
            continue;
          }

          for (size_t m=0; m<relation->roles.size(); m++) {
            TypeId type=relation->roles[m].type==typeIgnore ? relation->type : relation->roles[m].type;

            if (relation->layer!=layer) {
              continue;
            }

            DrawWayOutline(styleConfig,
                           type,
                           false,
                           false,
                           false,
                           false,
                           relation->roles[m].nodes);
          }
        }

        for (std::vector<Relation>::const_iterator relation=relationWays.begin();
             relation!=relationWays.end();
             ++relation) {

          if (relation->layer!=layer) {
            continue;
          }

          //std::cout << "Draw way relation " << relation->id << std::endl;
          for (size_t m=0; m<relation->roles.size(); m++) {
            TypeId type=relation->roles[m].type==typeIgnore ? relation->type : relation->roles[m].type;

            DrawWay(styleConfig,
                    type,
                    false,
                    false,
                    relation->roles[m].nodes);
          }
        }
      }
    }
  }

  void MapPainter::DrawWayLabels(const StyleConfig& styleConfig)
  {
    for (std::vector<Way>::const_iterator way=ways.begin();
         way!=ways.end();
         ++way) {

      if (!way->GetRefName().empty()) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->type);

        if (style==NULL ||
            magnification<style->GetMinMag() ||
            magnification>style->GetMaxMag() ||
            style->GetStyle()!=LabelStyle::contour) {
          continue;
        }

        if (!IsVisible(way->nodes)) {
          continue;
        }

        DrawContourLabel(draw,
                         *style,
                         way->GetRefName(),
                         way->nodes);
      }
      else if (!way->GetName().empty()) {
        const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->type);

        if (style==NULL ||
            magnification<style->GetMinMag() ||
            magnification>style->GetMaxMag() ||
            style->GetStyle()!=LabelStyle::contour) {
          continue;
        }

        if (!IsVisible(way->nodes)) {
          continue;
        }

        DrawContourLabel(draw,
                         *style,
                         way->GetName(),
                         way->nodes);
      }
    }

    std::set<size_t> labelMap;

    for (std::vector<Way>::const_iterator way=ways.begin();
         way!=ways.end();
         ++way) {

      if (!way->GetRefName().empty()) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->type);

        if (style==NULL ||
            magnification<style->GetMinMag() ||
            magnification>style->GetMaxMag() ||
            style->GetStyle()==LabelStyle::contour) {
            continue;
        }

        if (!IsVisible(way->nodes)) {
          continue;
        }

        double x,y;

        double xmin=way->nodes[0].lon;
        double xmax=way->nodes[0].lon;
        double ymin=way->nodes[0].lat;
        double ymax=way->nodes[0].lat;

        for (size_t j=1; j<way->nodes.size(); j++) {
          xmin=std::min(xmin,way->nodes[j].lon);
          xmax=std::max(xmax,way->nodes[j].lon);
          ymin=std::min(ymin,way->nodes[j].lat);
          ymax=std::max(ymax,way->nodes[j].lat);
        }

        TransformGeoToPixel(xmin,ymin,xmin,ymin);
        TransformGeoToPixel(xmax,ymax,xmax,ymax);

        x=xmin+(xmax-xmin)/2;
        y=ymin+(ymax-ymin)/2;

        size_t tx,ty;

        tx=(x-xmin)*20/(xmax-xmin);
        ty=(y-ymin)*20/(ymax-ymin);

        size_t tile=20*ty+tx;

        if (labelMap.find(tile)!=labelMap.end()) {
          break;
        }

        DrawLabel(draw,
                  magnification,
                  *style,
                  way->GetRefName(),
                  x,y);

        labelMap.insert(tile);
      }
      else if (!way->GetName().empty()) {
        const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->type);

        if (style==NULL ||
            magnification<style->GetMinMag() ||
            magnification>style->GetMaxMag() ||
            style->GetStyle()==LabelStyle::contour) {
          continue;
        }

        if (!IsVisible(way->nodes)) {
          continue;
        }

        double x,y;

        if (!GetCenterPixel(way->nodes,x,y)) {
          continue;
        }

        DrawLabel(draw,
                  magnification,
                  *style,
                  way->GetName(),
                  x,y);
      }
    }
  }

  void MapPainter::DrawNodes(const StyleConfig& styleConfig)
  {
    for (std::vector<Node>::const_iterator node=nodes.begin();
         node!=nodes.end();
         ++node) {
      const LabelStyle  *labelStyle=styleConfig.GetNodeLabelStyle(node->type);
      const IconStyle   *iconStyle=styleConfig.GetNodeIconStyle(node->type);
      const SymbolStyle *symbolStyle=iconStyle!=NULL ? NULL : styleConfig.GetNodeSymbolStyle(node->type);

      bool hasLabel=labelStyle!=NULL &&
                    magnification>=labelStyle->GetMinMag() &&
                    magnification<=labelStyle->GetMaxMag();

      bool hasSymbol=symbolStyle!=NULL &&
                     magnification>=symbolStyle->GetMinMag();

      bool hasIcon=iconStyle!=NULL &&
                   magnification>=iconStyle->GetMinMag();

      std::string label;

      nodesDrawnCount++;

      if (hasLabel) {
        for (size_t i=0; i<node->tags.size(); i++) {
          // TODO: We should make sure we prefer one over the other
          if (node->tags[i].key==tagName) {
            label=node->tags[i].value;
            break;
          }
          else if (node->tags[i].key==tagRef)  {
            label=node->tags[i].value;
          }
        }

        hasLabel=!label.empty();
      }

      if (hasIcon) {
        hasIcon=CheckImage(styleConfig,iconStyle->GetIcon());
      }

      if (!hasSymbol && !hasLabel && !hasIcon) {
        continue;
      }

      double x,y;

      x=(node->lon*gradtorad-hmin)*hscale;
      y=height-(atanh(sin(node->lat*gradtorad))-vmin)*vscale;

      if (hasLabel) {
        if (hasSymbol) {
          DrawLabel(draw,
                    magnification,
                    *labelStyle,
                    label,
                    x,y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
        }
        else if (hasIcon) {
          DrawLabel(draw,
                    magnification,
                    *labelStyle,
                    label,
                    x,y+14+5); // TODO: Better layout to real size of icon
        }
        else {
          DrawLabel(draw,
                    magnification,
                    *labelStyle,
                    label,
                    x,y);
        }
      }

      if (hasIcon) {
        DrawIcon(draw,iconStyle,x,y);
      }

      if (hasSymbol) {
        DrawSymbol(draw,symbolStyle,x,y);
      }
    }
  }

  void MapPainter::DrawAreaLabels(const StyleConfig& styleConfig)
  {
    for (std::vector<Way>::const_iterator area=areas.begin();
         area!=areas.end();
         ++area) {

      const LabelStyle *style=styleConfig.GetAreaLabelStyle(area->type);

      if (style==NULL ||
          magnification<style->GetMinMag() ||
          magnification>style->GetMaxMag()) {
        continue;
      }

      if (!area->GetName().empty()) {
        if (!IsVisible(area->nodes)) {
          continue;
        }

        double x,y;

        if (!GetCenterPixel(area->nodes,x,y)) {
          continue;
        }

        DrawLabel(draw,
                  magnification,
                  *style,
                  area->GetName(),
                  x,y);
      }
      else if (!area->GetRefName().empty()) {
        if (!IsVisible(area->nodes)) {
          continue;
        }

        double x,y;

        if (!GetCenterPixel(area->nodes,x,y)) {
          continue;
        }

        DrawLabel(draw,
                  magnification,
                  *style,
                  area->GetRefName(),
                  x,y);
      }
    }
  }

  void MapPainter::DrawPOIWays(const StyleConfig& styleConfig)
  {
    for (std::list<Way>::const_iterator way=poiWays.begin();
         way!=poiWays.end();
         ++way) {

      if (way->IsArea()) {
        std::cerr << "POI way is area, skipping..." << std::endl;
        continue;
      }

      DrawWay(styleConfig,
              way->type,
              way->flags & Way::isBridge,
              way->flags & Way::isTunnel,
              way->nodes);


      waysDrawnCount++;

      //nodesAllCount+=way->nodes.size();

      cairo_stroke(draw);
    }
  }

  void MapPainter::DrawPOINodes(const StyleConfig& styleConfig)
  {
    for (std::list<Node>::const_iterator node=poiNodes.begin();
         node!=poiNodes.end();
         ++node) {
      if (node->lon<lonMin ||
          node->lon>lonMax ||
          node->lat<latMin ||
          node->lat>latMax) {
        continue;
      }

      const SymbolStyle *style=styleConfig.GetNodeSymbolStyle(node->type);

      if (style==NULL ||
          magnification<style->GetMinMag()) {
        continue;
      }

      double x,y;

      TransformGeoToPixel(node->lon,node->lat,x,y);

      DrawSymbol(draw,style,x,y);

      nodesDrawnCount++;
    }
  }

  void MapPainter::DrawPOINodeLabels(const StyleConfig& styleConfig)
  {
    for (std::list<Node>::const_iterator node=poiNodes.begin();
         node!=poiNodes.end();
         ++node) {
      if (node->lon<lonMin ||
          node->lon>lonMax ||
          node->lat<latMin ||
          node->lat>latMax) {
        continue;
      }

      for (size_t i=0; i<node->tags.size(); i++) {
        // TODO: We should make sure we prefer one over the other
        if (node->tags[i].key==tagName) {
          const LabelStyle *style=styleConfig.GetNodeLabelStyle(node->type);

          if (style==NULL ||
              magnification<style->GetMinMag() ||
              magnification>style->GetMaxMag()) {
            continue;
          }

          double x,y;

          TransformGeoToPixel(node->lon,node->lat,x,y);

          DrawLabel(draw,
                    magnification,
                    *style,
                    node->tags[i].value,
                    x,y);
        }
        else if (node->tags[i].key==tagRef)  {
          const LabelStyle *style=styleConfig.GetNodeRefLabelStyle(node->type);

          if (style==NULL ||
              magnification<style->GetMinMag() ||
              magnification>style->GetMaxMag()) {
            continue;
          }

          double x,y;

          TransformGeoToPixel(node->lon,node->lat,x,y);

          DrawLabel(draw,
                    magnification,
                    *style,
                    node->tags[i].value,
                    x,y);
        }
      }
    }
  }

  bool MapPainter::DrawMap(const StyleConfig& styleConfig,
                           double lon, double lat,
                           double magnification,
                           size_t width, size_t height,
                           cairo_surface_t *image,
                           cairo_t *draw)
  {
    double gradtorad=2*M_PI/360;

    nodesDrawnCount=0;
    areasDrawnCount=0;
    waysDrawnCount=0;

    styleCount=styleConfig.GetStyleCount();

    this->draw=draw;

    std::cout << "---" << std::endl;
    std::cout << "Showing " << lon <<", " << lat << " with magnification " << magnification << "x" << "/" << log(magnification)/log(2) << " for area " << width << "x" << height << std::endl;

    StopClock           overallTimer;

    //
    // calculation of bounds and scaling factors
    //

    // Make a copy of the context information
    this->lon=lon;
    this->lat=lat;
    this->width=width;
    this->height=height;
    this->magnification=magnification;

    // Get bounding dimensions and copy them to the context information, too
    GetDimensions(lon,lat,magnification,width,height,lonMin,latMin,lonMax,latMax);

    std::cout << "Dimension: " << lonMin << " " << latMin << " " << lonMax << " " << latMax << std::endl;

    hmin=lonMin*gradtorad;
    hmax=lonMax*gradtorad;
    vmin=atanh(sin(latMin*gradtorad));
    vmax=atanh(sin(latMax*gradtorad));

    hscale=(width-1)/(hmax-hmin);
    vscale=(height-1)/(vmax-vmin);

    // Width of an pixel in meter
    double d=(lonMax-lonMin)*gradtorad;
    double pixelSize=d*180*60/M_PI*1852.216/width;

    /*
    std::cout << "Box (grad) h: " << lonMin << "-" << lonMax << " v: " << latMin <<"-" << latMax << std::endl;
    std::cout << "Box (merc) h: " << hmin << "-" << hmax << " v: " << vmin <<"-" << vmax << std::endl;
    std::cout << "hscale: " << hscale << " vscale: " << vscale << std::endl;
    std::cout << "d: " << d << " " << d*180*60/M_PI << std::endl;
    std::cout << "The complete screen are " << d*180*60/M_PI*1852.216 << " meters" << std::endl;
    std::cout << "1 pixel are " << pixelSize << " meters" << std::endl;
    std::cout << "20 meters are " << 20/(d*180*60/M_PI*1852.216/width) << " pixels" << std::endl;
    */

    StopClock dataRetrievalTimer;

    nodes.clear();
    ways.clear();
    areas.clear();
    relationWays.clear();
    relationAreas.clear();

    database.GetObjects(styleConfig,
                        lonMin,latMin,lonMax,latMax,
                        magnification,
                        ((size_t)ceil(Log2(magnification)))+4,
                        2000,
                        3000,
                        2000,
                        nodes,
                        ways,
                        areas,
                        relationWays,
                        relationAreas);

    dataRetrievalTimer.Stop();

    StopClock presetTimer;

    //
    // Setup and Precalculation
    //

    lineWidth.resize(styleCount,0);
    borderWidth.resize(styleCount,0);
    outline.resize(styleCount,false);

    // Calculate real line width and outline size for each way line style

    for (size_t i=0; i<styleCount; i++) {
      const LineStyle *lineStyle=styleConfig.GetWayLineStyle(i);

      if (lineStyle!=NULL) {
        lineWidth[i]=lineStyle->GetWidth()/pixelSize;
        if (lineWidth[i]<lineStyle->GetMinPixel()) {
          lineWidth[i]=lineStyle->GetMinPixel();
        }
        else if (lineWidth[i]>lineStyle->GetMaxPixel()) {
          lineWidth[i]=lineStyle->GetMaxPixel();
        }

        outline[i]=lineStyle->GetOutline()>0 && magnification>=magRegion;

        const LabelStyle *labelStyle=styleConfig.GetWayNameLabelStyle(i);

        if (lineStyle->GetStyle()==LineStyle::normal &&
            labelStyle!=NULL &&
            labelStyle->GetStyle()==LabelStyle::contour &&
            labelStyle->GetMinMag()<=magnification &&
            labelStyle->GetMaxMag()>=magnification &&
            lineWidth[i]<9.0 &&
            outline[i]) {
          lineWidth[i]=9.0;
        }
      }

      const LineStyle *borderStyle=styleConfig.GetAreaBorderStyle(i);

      if (borderStyle!=NULL) {
        borderWidth[i]=borderStyle->GetWidth()/pixelSize;
        if (borderWidth[i]<borderStyle->GetMinPixel()) {
          borderWidth[i]=borderStyle->GetMinPixel();
        }
        else if (lineWidth[i]>borderStyle->GetMaxPixel()) {
          borderWidth[i]=borderStyle->GetMaxPixel();
        }
      }
    }

    //
    // Calculate available layers for ways
    //

    for (size_t i=0; i<11; i++) {
      wayLayers[i]=false;
    }

    for (std::vector<Way>::const_iterator way=ways.begin();
         way!=ways.end();
         ++way) {
      if (way->layer>=-5 && way->layer<=5) {
        wayLayers[way->layer+5]=true;
      }
    }

    for (size_t i=0; i<11; i++) {
      relationWayLayers[i]=false;
    }

    for (std::vector<Relation>::const_iterator relation=relationWays.begin();
         relation!=relationWays.end();
         ++relation) {
      if (relation->layer>=-5 && relation->layer<=5) {
        relationWayLayers[relation->layer+5]=true;
      }
    }

    //
    // Calculate available layers for areas
    //

    for (size_t i=0; i<11; i++) {
      areaLayers[i]=false;
    }

    for (std::vector<Way>::const_iterator area=areas.begin();
         area!=areas.end();
         ++area) {
      const FillStyle *style=styleConfig.GetAreaFillStyle(area->type,
                                                          area->flags & Way::isBuilding);

      if (style!=NULL &&
          style->GetLayer()>=-5 &&
          style->GetLayer()<=5) {
        areaLayers[style->GetLayer()+5]=true;
      }
    }

    for (size_t i=0; i<11; i++) {
      relationAreaLayers[i]=false;
    }

    for (std::vector<Relation>::const_iterator relation=relationAreas.begin();
         relation!=relationAreas.end();
         ++relation) {
      const FillStyle *style=styleConfig.GetAreaFillStyle(relation->type,
                                                          false/*relation->flags & Way::isBuilding*/);

      if (style!=NULL &&
          style->GetLayer()>=-5 &&
          style->GetLayer()<=5) {
        relationAreaLayers[style->GetLayer()+5]=true;
      }
    }

    //
    //
    // Drawing setup

    cairo_save(draw);

    cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);

    cairo_set_source_rgb(draw,241.0/255,238.0/255,233.0/255);
    cairo_rectangle(draw,0,0,width,height);
    cairo_fill(draw);

    presetTimer.Stop();

    StopClock drawingTimer;

    //
    // Draw areas
    //

    StopClock areasTimer;

    DrawAreas(styleConfig);

    areasTimer.Stop();

    //
    // Drawing ways
    //

    StopClock pathsTimer;

    DrawWays(styleConfig);

    pathsTimer.Stop();

    //
    // Path labels
    //

    // TODO: Draw labels only if there is a style for the current zoom level
    // that requires labels

    StopClock pathLabelsTimer;

    DrawWayLabels(styleConfig);

    pathLabelsTimer.Stop();

    //
    // Nodes symbols & Node labels
    //

    StopClock nodesTimer;

    DrawNodes(styleConfig);

    nodesTimer.Stop();

    //
    // Area labels
    //

    StopClock areaLabelsTimer;

    DrawAreaLabels(styleConfig);

    areaLabelsTimer.Stop();

    //
    // POI ways (aka routes)
    //


    StopClock routesTimer;

    DrawPOIWays(styleConfig);

    routesTimer.Stop();

    //
    // POI Nodes
    //

    StopClock poisTimer;

    DrawPOINodes(styleConfig);

    //
    // POI Node labels
    //

    DrawPOINodeLabels(styleConfig);

    //cairo_restore(draw);

    poisTimer.Stop();

    drawingTimer.Stop();
    overallTimer.Stop();

    std::cout << "Nodes: " << nodesDrawnCount << "/" << nodes.size()+poiNodes.size() << " ";
    if (nodes.size()+poiNodes.size()>0) {
      std::cout << "(" << nodesDrawnCount*100/(nodes.size()+poiNodes.size()) << "%) ";
    }

    std::cout << " ways: " << waysDrawnCount << "/" << ways.size()+poiWays.size() << " ";
    if (ways.size()+poiWays.size()>0) {
      std::cout << "(" << waysDrawnCount*100/(ways.size()+poiWays.size()) << "%) ";
    }

    std::cout << " areas: " << areasDrawnCount << "/" << areas.size() << " ";
    if (areas.size()>0) {
      std::cout << "(" << areasDrawnCount*100/areas.size() << "%) ";
    }
    std::cout << std::endl;

    std::cout << "All: " << overallTimer;
    std::cout << " Data: " << dataRetrievalTimer;
    std::cout << " Preset: " << presetTimer;
    std::cout << " Draw: " << drawingTimer << std::endl;
    std::cout << "Areas: " << areasTimer <<"/" << areaLabelsTimer;
    std::cout << " Paths: " << pathsTimer << "/" << pathLabelsTimer;
    std::cout << " Nodes: " << nodesTimer;
    std::cout << " POIs: " << poisTimer << "/" << routesTimer << std::endl;

    return true;
  }

  bool MapPainter::PrintMap(const StyleConfig& styleConfig,
                            double lon, double lat,
                            double magnification,
                            size_t width, size_t height)
  {
    cairo_surface_t *image=cairo_image_surface_create(CAIRO_FORMAT_RGB24,width,height);
    cairo_t         *draw=cairo_create(image);

    DrawMap(styleConfig,
            lon,lat,magnification,width,height,
            image,draw);

    std::cout << "Saving..." << std::endl;

    cairo_surface_write_to_png(image,"map.png");

    cairo_destroy(draw);
    cairo_surface_destroy(image);

    return true;
  }

  bool MapPainter::TransformPixelToGeo(int x, int y,
                                       double centerLon, double centerLat,
                                       double magnification,
                                       size_t width, size_t height,
                                       double& outLon, double& outLat)
  {
    double lonMin,latMin,lonMax,latMax;

    GetDimensions(centerLon,centerLat,
                  magnification,
                  width,height,
                  lonMin,latMin,lonMax,latMax);

    outLon=lonMin+(lonMax-lonMin)*x/width;

    // This transformation is currently only valid for big magnifications
    // since it does not take the mercator (back-)transformation into account!
    outLat=latMin+(latMax-latMin)*y/height;

    return true;
  }

  bool MapPainter::TransformGeoToPixel(double lon, double lat,
                                       double centerLon, double centerLat,
                                       double magnification,
                                       size_t width, size_t height,
                                       double &x, double& y)
  {
    double lonMin,latMin,lonMax,latMax,hmin,hmax,vmin,vmax,hscale,vscale;

    GetDimensions(centerLon,centerLat,
                  magnification,
                  width,height,
                  lonMin,latMin,lonMax,latMax);

    hmin=lonMin*gradtorad;
    hmax=lonMax*gradtorad;
    vmin=atanh(sin(latMin*gradtorad));
    vmax=atanh(sin(latMax*gradtorad));

    hscale=(width-1)/(hmax-hmin);
    vscale=(height-1)/(vmax-vmin);

    x=(lon*gradtorad-hmin)*hscale;
    y=height-(atanh(sin(lat*gradtorad))-vmin)*vscale;

    return true;
  }
}

