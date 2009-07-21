/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include "MapPainter.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <list>
#include <cstdlib>

static const double gradtorad=2*M_PI/360;

/* Returns Euclidean distance between two points */
static double two_points_distance (cairo_path_data_t *a, cairo_path_data_t *b)
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
static parametrization_t* parametrize_path (cairo_path_t *path)
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


typedef void (*transform_point_func_t) (void *closure, double *x, double *y);

/* Project a path using a function. Each point of the path (including
  * Bezier control points) is passed to the function for transformation.
*/
static void transform_path (cairo_path_t *path, transform_point_func_t f, void *closure)
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
static void point_on_path (parametrized_path_t *param, double *x, double *y)
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
static void map_path_onto (cairo_t *cr, cairo_path_t *path)
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

static void draw_twisted (cairo_t *cr, double x, double y, const char *text)
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

static double relevantPosDeriviation=2.0;
static double relevantSlopeDeriviation=0.1;

MapPainter::MapPainter(const Database& database)
 : database(database)
{
  // no code
}

MapPainter::~MapPainter()
{
  // no code
}

void MapPainter::DrawLabel(cairo_t* draw,
                           const LabelStyle& style,
                           const std::string& text,
                           double x, double y)
{
  cairo_text_extents_t textExtents;

  cairo_text_extents(draw,text.c_str(),&textExtents);

  // TODO: If the point is offscreen move it into the screen...

  if (style.GetStyle()==LabelStyle::normal) {
    cairo_font_extents_t fontExtents;

    cairo_select_font_face(draw,
                           "sans-serif",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(draw,style.GetSize()*9.0);
    cairo_font_extents(draw,&fontExtents);

    cairo_set_source_rgba(draw,
                          style.GetTextR(),
                          style.GetTextG(),
                          style.GetTextB(),
                          style.GetTextA());

    cairo_move_to(draw,x-textExtents.width/2+textExtents.x_bearing,
                  y-textExtents.height/2+fontExtents.ascent+textExtents.y_bearing);
    cairo_show_text(draw,text.c_str());
    cairo_stroke(draw);
  }
  else if (style.GetStyle()==LabelStyle::plate) {
    static const double outerWidth = 4;
    static const double innerWidth = 2;

    cairo_font_extents_t fontExtents;

    cairo_select_font_face(draw,
                           "sans-serif",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(draw,style.GetSize()*9.0);
    cairo_font_extents(draw,&fontExtents);

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
    cairo_font_options_t *font_options;
    cairo_font_extents_t fontExtents;

    cairo_save(draw);

    font_options = cairo_font_options_create();
    cairo_font_options_set_hint_style(font_options,CAIRO_HINT_STYLE_DEFAULT);
    cairo_font_options_set_hint_metrics(font_options,CAIRO_HINT_METRICS_DEFAULT);

    cairo_set_font_options (draw,font_options);
    cairo_font_options_destroy(font_options);

    cairo_select_font_face(draw,
                           "sans-serif",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(draw,style.GetSize()*9.0);
    cairo_font_extents(draw,&fontExtents);

    cairo_move_to(draw,x-textExtents.width/2+textExtents.x_bearing,
                  y-textExtents.height/2+fontExtents.ascent+textExtents.y_bearing);

    cairo_text_path(draw,text.c_str());
    cairo_set_source_rgba(draw,1,1,1,
                          style.GetTextA());
    cairo_set_line_width(draw,2.0);
    cairo_stroke_preserve(draw);
    cairo_set_source_rgba(draw,
                          style.GetTextR(),
                          style.GetTextG(),
                          style.GetTextB(),
                          style.GetTextA());
    cairo_fill(draw);

    cairo_restore(draw);
  }
}

void MapPainter::DrawContourLabel(cairo_t* draw,
                                  const LabelStyle& style,
                                  const std::string& text,
                                  const std::vector<Point>& nodes,
                                  double hmin, double vmin,
                                  double hscale, double vscale,
                                  double height)
{
  cairo_font_extents_t fontExtents;

  cairo_select_font_face(draw,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(draw,style.GetSize()*9.0);
  cairo_font_extents(draw,&fontExtents);

  cairo_new_path(draw);
  double length=0;
  double xo=0,yo=0;
  double x=0,y=0;
  if (nodes[0].lon<nodes[nodes.size()-1].lon) {
    for (size_t j=0; j<nodes.size(); j++) {
      xo=x;
      yo=y;

      x=(nodes[j].lon*gradtorad-hmin)*hscale;
      y=height-(atanh(sin(nodes[j].lat*gradtorad))-vmin)*vscale;

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

      x=(nodes[nodes.size()-j-1].lon*gradtorad-hmin)*hscale;
      y=height-(atanh(sin(nodes[nodes.size()-j-1].lat*gradtorad))-vmin)*vscale;

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

  cairo_text_extents(draw,text.c_str(),&textExtents);

  if (length>=textExtents.width) {
    cairo_set_source_rgba(draw,
                          style.GetTextR(),
                          style.GetTextG(),
                          style.GetTextB(),
                          style.GetTextA());

    draw_twisted(draw,(length-textExtents.width)/2+textExtents.x_bearing,
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
    cairo_set_source_rgb(draw,style->GetFillR(),style->GetFillG(),style->GetFillB());
    cairo_set_line_width(draw,1);

    cairo_rectangle(draw,
                    x-style->GetSize()/2,y-style->GetSize()/2,
                    style->GetSize(),style->GetSize());
    cairo_fill(draw);
    break;
  case SymbolStyle::circle:
    cairo_set_source_rgb(draw,style->GetFillR(),style->GetFillG(),style->GetFillB());
    cairo_set_line_width(draw,1);

    cairo_arc(draw,
              x,y,
              style->GetSize(),
              0,2*M_PI);
    cairo_fill(draw);
    break;
  }
}

void MapPainter::GetPixelDelta(double lonA, double latA,
                               double lonB, double latB,
                               double magnification,
                               size_t width, size_t height,
                               double& dx, double& dy)
{
  double lonMin,lonMax,latMin,latMax;
  double hmin,hmax;
  double vmin,vmax;
  double hscale,vscale;

  GetDimensions(lonA,latA,magnification,width,height,lonMin,latMin,lonMax,latMax);

  hmin=lonMin*gradtorad;
  hmax=lonMax*gradtorad;
  vmin=atanh(sin(latMin*gradtorad));
  vmax=atanh(sin(latMax*gradtorad));

  hscale=(width-1)/(hmax-hmin);
  vscale=(height-1)/(vmax-vmin);

  std::cout << lonA << "," << latA << " <=> " << lonB << "," << latB << std::endl;

  dx=(lonB*gradtorad-hmin)*hscale-(lonA*gradtorad-hmin)*hscale;
  dy=(atanh(sin(latB*gradtorad))-vmin)*vscale-(atanh(sin(latA*gradtorad))-vmin)*vscale;

  std::cout << "DELTA: " << dx << " - " << dy << std::endl;
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

bool MapPainter::DrawMap(const StyleConfig& styleConfig,
                         double lon, double lat,
                         double magnification,
                         size_t width, size_t height,
                         cairo_surface_t *image,
                         cairo_t *draw)
{
  size_t              styleCount=styleConfig.GetStyleCount();
  std::list<Node>     nodes;
  std::list<Way>      ways;
  size_t              nodesOut=0;
  size_t              nodesAllCount=0;
  size_t              nodesDrawnCount=0;
  std::ifstream       in;
  std::vector<size_t> distribution; // Distribution of DrawStyle in loaded data
  std::vector<size_t> drawnDistribution; // Distribution of DrawStyle in drawn data
  std::vector<size_t> drawnNodeDistribution; // Distribution of nodes per DrawStyle in drawn data
  std::vector<double> lineWidth; // line with for this way style
  std::vector<bool>   outline; // We draw an outline for this way style
  std::vector<bool>   drawNode; // This node will be drawn
  std::vector<bool>   outNode; // This nodes is out of the visible area
  std::vector<double> nodeX;
  std::vector<double> nodeY;
  bool                areaLayers[11];
  bool                wayLayers[11];
  double              lonMin,lonMax,latMin,latMax;
  double              hmin,hmax;
  double              vmin,vmax;

  double              hscale,vscale;

  double              gradtorad=2*M_PI/360;
  double              longDash[]= {7,3};
  double              dotted[]= {1,3};
  double              lineDot[]= {7,3,1,3};

  std::cout << "Showing " << lon <<", " << lat << " with magnification " << magnification << "x" << std::endl;

  //
  // calculation of bounds and scaling factors
  //

  GetDimensions(lon,lat,magnification,width,height,lonMin,latMin,lonMax,latMax);

  hmin=lonMin*gradtorad;
  hmax=lonMax*gradtorad;
  vmin=atanh(sin(latMin*gradtorad));
  vmax=atanh(sin(latMax*gradtorad));

  std::cout << "Box (grad) h: " << lonMin << "-" << lonMax << " v: " << latMin <<"-" << latMax << std::endl;
  std::cout << "Box (merc) h: " << hmin << "-" << hmax << " v: " << vmin <<"-" << vmax << std::endl;

  hscale=(width-1)/(hmax-hmin);
  vscale=(height-1)/(vmax-vmin);

  std::cout << "hscale: " << hscale << " vscale: " << vscale << std::endl;

  double d=(lonMax-lonMin)*gradtorad;
  double pixelSize=d*180*60/M_PI*1852.216/width;

  std::cout << "d: " << d << " " << d*180*60/M_PI << std::endl;

  std::cout << "The complete screen are " << d*180*60/M_PI*1852.216 << " meters" << std::endl;

  std::cout << "1 pixel are " << pixelSize << " meters" << std::endl;

  std::cout << "20 meters are " << 20/(d*180*60/M_PI*1852.216/width) << " pixels" << std::endl;

  database.GetObjects(styleConfig,
                      lonMin,latMin,lonMax,latMax,
                      magnification,
                      5000,
                      nodes,
                      ways);

  std::cout << "Nodes: " << nodes.size() << " ways: " << ways.size() << std::endl;

  //
  // Setup and Precalculation
  //

  distribution.resize(styleCount,0);
  drawnDistribution.resize(styleCount,0);
  drawnNodeDistribution.resize(styleCount,0);
  lineWidth.resize(styleCount,0);
  outline.resize(styleCount,false);
  drawNode.resize(10000,true); // TODO: Calculate matching size
  outNode.resize(10000,true); // TODO: Calculate matching size
  nodeX.resize(10000,true); // TODO: Calculate matching size
  nodeY.resize(10000,true); // TODO: Calculate matching size

  // Calculate real line width and outline size for each way line style

  for (size_t i=0; i<styleCount; i++) {
    const LineStyle *style=styleConfig.GetWayLineStyle(i);

    if (style!=NULL) {
      lineWidth[i]=style->GetWidth()/pixelSize;
      if (lineWidth[i]<style->GetMinPixel()) {
        lineWidth[i]=style->GetMinPixel();
        outline[i]=style->GetOutline()>0 && magnification>=magRegion;
      }
      else if (lineWidth[i]>style->GetMaxPixel()) {
        lineWidth[i]=style->GetMaxPixel();
        outline[i]=style->GetOutline()>0 && magnification>=magRegion;
      }
      else {
        outline[i]=style->GetOutline()>0 && magnification>=magRegion;
      }

      const LabelStyle *labelStyle=styleConfig.GetWayNameLabelStyle(i);

      if (labelStyle!=NULL &&
          labelStyle->GetStyle()==LabelStyle::contour &&
          labelStyle->GetMinMag()<=magnification &&
          lineWidth[i]<9.0 &&
          outline[i]) {
        lineWidth[i]=9.0;
      }
    }
  }

  // Calculate available layers for ways and areas

  for (size_t i=0; i<11; i++) {
    areaLayers[i]=false;
  }

  for (size_t i=0; i<11; i++) {
    wayLayers[i]=false;
  }


  for (std::list<Way>::const_iterator area=ways.begin();
       area!=ways.end();
       ++area) {
    if (area->IsArea()) {
      const FillStyle *style=styleConfig.GetAreaFillStyle(area->type,
                                                          area->flags & Way::isBuilding);

      if (style->GetLayer()>=-5 && style->GetLayer()<=5) {
        areaLayers[style->GetLayer()+5]=true;
      }
    }
    else {
      if (area->layer>=-5 && area->layer<=5) {
        wayLayers[area->layer+5]=true;
      }
    }
  }

  //
  //
  // Drawing setup

  //std::cout << "Drawing..." << std::endl;

  cairo_save(draw);

  cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);

  cairo_set_source_rgb(draw,241.0/255,238.0/255,233.0/255);
  cairo_rectangle(draw,0,0,width,height);
  cairo_fill(draw);

  cairo_font_options_t *font_options;
  cairo_font_extents_t fontExtents;

  font_options = cairo_font_options_create ();
  cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);

  cairo_set_font_options (draw, font_options);
  cairo_font_options_destroy (font_options);

  cairo_font_extents(draw,&fontExtents);

  cairo_select_font_face(draw,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(draw,9);

  //
  // Draw areas
  //

  //std::cout << "Draw areas..." << std::endl;

  cairo_save(draw);
  for (size_t l=0; l<11; l++) {
    int layer=l-5;

    if (!areaLayers[l]) {
      continue;
    }

    //std::cout << "Drawing layer " << layer << std::endl;

    for (std::list<Way>::const_iterator area=ways.begin();
         area!=ways.end();
         ++area) {

      if (!area->IsArea()) {
        continue;
      }

      const FillStyle *style=styleConfig.GetAreaFillStyle(area->type,
                                                          area->flags & Way::isBuilding);

      if (style==NULL || style->GetLayer()!=layer) {
        continue;
      }

      cairo_set_source_rgb(draw,style->GetFillR(),style->GetFillG(),style->GetFillB());
      cairo_set_line_width(draw,1);

      for (size_t i=0; i<area->nodes.size(); i++) {
        drawNode[i]=true;
      }

      for (size_t i=1; i<area->nodes.size()-1; i++) {
        if (std::abs((area->nodes[i].lon-area->nodes[i-1].lon)/
                     (area->nodes[i].lat-area->nodes[i-1].lat)-
                     (area->nodes[i+1].lon-area->nodes[i].lon)/
                     (area->nodes[i+1].lat-area->nodes[i].lat))<relevantSlopeDeriviation) {
          drawNode[i]=false;
        }
      }

      for (size_t i=0; i<area->nodes.size(); i++) {
        if (drawNode[i]) {
          nodeX[i]=(area->nodes[i].lon*gradtorad-hmin)*hscale;
          nodeY[i]=height-(atanh(sin(area->nodes[i].lat*gradtorad))-vmin)*vscale;
        }
      }

      for (size_t i=1; i<area->nodes.size()-1; i++) {
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

      for (size_t i=0; i<area->nodes.size(); i++) {
        if (i==0) {
          cairo_move_to(draw,nodeX[i],nodeY[i]);
          drawnNodeDistribution[area->type]++;
          nodesDrawnCount++;
        }
        else if (drawNode[i]) {
          cairo_line_to(draw,nodeX[i],nodeY[i]);
          drawnNodeDistribution[area->type]++;
          nodesDrawnCount++;
        }

        nodesAllCount++;
      }

      cairo_fill(draw);
    }
  }


  cairo_restore(draw);

  //
  // Drawing paths
  //

  //std::cout << "Draw path outlines..." << std::endl;

  for (size_t l=0; l<11; l++) {
    int8_t layer=l-5;
    // Potential path outline

    if (!wayLayers[l]) {
      continue;
    }

    //std::cout << "Drawing layer " << (int)layer << std::endl;

    cairo_save(draw);
    for (std::list<Way>::const_iterator way=ways.begin();
         way!=ways.end();
         ++way) {

      if (way->IsArea()) {
        continue;
      }

      const LineStyle *style=styleConfig.GetWayLineStyle(way->type);

      if (style==NULL ||
          (!outline[(size_t)way->type] &&
           !(way->flags & Way::isBridge) &&
           !(way->flags & Way::isTunnel)) ||
           way->layer!=layer) {
        continue;
      }

      if (way->flags & Way::isBridge) {
        cairo_set_dash(draw,NULL,0,0);
        cairo_set_source_rgb(draw,0.0,0.0,0.0);
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
      }
      else if (way->flags & Way::isTunnel) {
        double tunnel[2];

        tunnel[0]=7+lineWidth[(size_t)way->type]+2*style->GetOutline();
        tunnel[1]=7+lineWidth[(size_t)way->type]+2*style->GetOutline();

        cairo_set_dash(draw,tunnel,2,0);
        cairo_set_source_rgb(draw,0.5,0.5,0.5);
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
      }
      else {
        cairo_set_dash(draw,NULL,0,0);
        cairo_set_source_rgb(draw,0.5,0.5,0.5);
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_BUTT);
      }
      cairo_set_line_width(draw,lineWidth[(size_t)way->type]+2*style->GetOutline());

      for (size_t i=0; i<way->nodes.size(); i++) {
        drawNode[i]=true;
        outNode[i]=false;
      }

      /*
      // Check which nodes or not visible in the given area
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

      for (size_t i=1; i+1<way->nodes.size(); i++) {
        if (drawNode[i] &&
            std::abs((way->nodes[i].lon-way->nodes[i-1].lon)/
                     (way->nodes[i].lat-way->nodes[i-1].lat)-
                     (way->nodes[i+1].lon-way->nodes[i].lon)/
                     (way->nodes[i+1].lat-way->nodes[i].lat))<relevantSlopeDeriviation) {
          drawNode[i]=false;
        }
      }

      for (size_t i=0; i<way->nodes.size(); i++) {
        if (drawNode[i]) {
          nodeX[i]=(way->nodes[i].lon*gradtorad-hmin)*hscale;
          nodeY[i]=height-(atanh(sin(way->nodes[i].lat*gradtorad))-vmin)*vscale;
        }
      }

      for (size_t i=1; i+1<way->nodes.size(); i++) {
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

      bool start=true;
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (drawNode[i]) {
          if (start) {
            cairo_move_to(draw,nodeX[i],nodeY[i]);
            start=false;
          }
          else {
            cairo_line_to(draw,nodeX[i],nodeY[i]);
          }

          drawnNodeDistribution[way->type]++;
          nodesDrawnCount++;
        }
      }
      nodesAllCount+=way->nodes.size();

      cairo_stroke(draw);

      if (!(way->flags & Way::startIsJoint)) {
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
        cairo_set_dash(draw,NULL,0,0);
        cairo_set_source_rgb(draw,0.5,0.5,0.5);
        cairo_set_line_width(draw,lineWidth[(size_t)way->type]+2*style->GetOutline());

        cairo_move_to(draw,nodeX[0],nodeY[0]);
        cairo_line_to(draw,nodeX[0],nodeY[0]);
        cairo_stroke(draw);
      }

      if (!(way->flags & Way::endIsJoint)) {
        cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
        cairo_set_dash(draw,NULL,0,0);
        cairo_set_source_rgb(draw,0.5,0.5,0.5);
        cairo_set_line_width(draw,lineWidth[(size_t)way->type]+2*style->GetOutline());

        cairo_move_to(draw,nodeX[way->nodes.size()-1],nodeY[way->nodes.size()-1]);
        cairo_line_to(draw,nodeX[way->nodes.size()-1],nodeY[way->nodes.size()-1]);
        cairo_stroke(draw);
      }
    }

    /*
    cairo_restore(draw);
  }

  //std::cout << "Draw paths..." << std::endl;

  for (size_t l=0; l<11; l++) {
    int8_t layer=l-5;
    // Inner path fill

    if (!wayLayers[l]) {
      continue;
    } */

    //std::cout << "Drawing layer " << (int)layer << std::endl;

    cairo_save(draw);
    for (std::list<Way>::const_iterator way=ways.begin();
         way!=ways.end();
         ++way) {

      if (way->IsArea()) {
        continue;
      }

      const LineStyle *style=styleConfig.GetWayLineStyle(way->type);

      if (style==NULL || way->layer!=layer) {
        continue;
      }

      cairo_set_source_rgba(draw,
                            style->GetLineR(),
                            style->GetLineG(),
                            style->GetLineB(),
                            style->GetLineA());

      cairo_set_line_width(draw,lineWidth[(size_t)way->type]);
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);

      switch (style->GetStyle()) {
      case LineStyle::none:
        // way should not be visible in this case!
        assert(false);
        break;
      case LineStyle::normal:
        cairo_set_dash(draw,NULL,0,0);
        break;
      case LineStyle::longDash:
        cairo_set_dash(draw,longDash,2,0);
        break;
      case LineStyle::dotted:
        cairo_set_dash(draw,dotted,2,0);
        break;
      case LineStyle::lineDot:
        cairo_set_dash(draw,lineDot,4,0);
        break;
      }

      // First we draw all nodes
      for (size_t i=0; i<way->nodes.size(); i++) {
        drawNode[i]=true;
        outNode[i]=false;
      }

      // Ignore all nodes which are on a direct line between the previous and the next
      // node (minimal deriviation of slopes)
      for (size_t i=1; i+1<way->nodes.size(); i++) {
        if (drawNode[i] &&
            std::abs((way->nodes[i].lon-way->nodes[i-1].lon)/
                     (way->nodes[i].lat-way->nodes[i-1].lat)-
                     (way->nodes[i+1].lon-way->nodes[i].lon)/
                     (way->nodes[i+1].lat-way->nodes[i].lat))<relevantSlopeDeriviation) {
          drawNode[i]=false;
        }
      }

      // Calculate screen position for all points left
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (drawNode[i]) {
          nodeX[i]=(way->nodes[i].lon*gradtorad-hmin)*hscale;
          nodeY[i]=height-(atanh(sin(way->nodes[i].lat*gradtorad))-vmin)*vscale;
        }
      }

      // Now ignore all points which are close to another point of the same way
      for (size_t i=1; i+1<way->nodes.size(); i++) {
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

      /*
      // Check which nodes or not visible in the given area
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (way->nodes[i].lon<lonMin || way->nodes[i].lon>lonMax ||
            way->nodes[i].lat<latMin || way->nodes[i].lat>latMax){
          outNode[i]=true;
        }
      }

      if (drawNode[0] && outNode[1]) {
        drawNode[0]=false;
      }

      for (size_t i=1; i<way->nodes.size()-1; i++) {
        if (drawNode[i-1] && outNode[i-1] && drawNode[i+1] && outNode[i+1]) {
          drawNode[i]=false;
        }
      }

      if (drawNode[way->nodes.size()-2] && outNode[way->nodes.size()-2]) {
        drawNode[way->nodes.size()-1]=false;
      } */

      bool start=true;
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (drawNode[i]) {
          if (start) {
            cairo_move_to(draw,nodeX[i],nodeY[i]);
            start=false;
          }
          else  {
            cairo_line_to(draw,nodeX[i],nodeY[i]);
          }
          drawnNodeDistribution[way->type]++;
          nodesDrawnCount++;
        }
      }
      nodesAllCount+=way->nodes.size();

      cairo_stroke(draw);

      /*
      for (size_t i=0; i<way->nodes.size(); i++) {
        double x,y;

        x=(way->nodes[i].lon*gradtorad-hmin)*hscale;
        y=height-(atanh(sin(way->nodes[i].lat*gradtorad))-vmin)*vscale;

        if (drawNode[i]) {
          cairo_set_source_rgb(draw,0,1,0);
        }
        else {
          cairo_set_source_rgb(draw,1,0,0);
        }
        cairo_arc(draw,
                  x,y,
                  1.5,
                  0,2*M_PI);
        cairo_fill(draw);
      }*/
    }
    cairo_restore(draw);
  }

  // Path labels

  //std::cout << "Draw path labels..." << std::endl;

  cairo_save(draw);

  for (std::list<Way>::const_iterator way=ways.begin();
       way!=ways.end();
       ++way) {

    if (way->IsArea()) {
      continue;
    }

    for (size_t i=0; i<way->tags.size(); i++) {
      if (way->tags[i].key==tagRef) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->type);

        if (style==NULL || magnification<style->GetMinMag()) {
          continue;
        }

        if (style->GetStyle()==LabelStyle::contour) {
          DrawContourLabel(draw,
                           *style,
                           way->tags[i].value,
                           way->nodes,
                           hmin,vmin,hscale,vscale,height);
        }
      }
      else if (way->tags[i].key==tagName) {
        const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->type);

        if (style==NULL || magnification<style->GetMinMag()) {
          continue;
        }

        if (style->GetStyle()==LabelStyle::contour) {
          DrawContourLabel(draw,
                           *style,
                           way->tags[i].value,
                           way->nodes,
                           hmin,vmin,hscale,vscale,height);
        }
      }
    }
  }

  std::set<size_t> labelMap;

  for (std::list<Way>::const_iterator way=ways.begin();
       way!=ways.end();
       ++way) {

    if (way->IsArea()) {
      continue;
    }

    for (size_t i=0; i<way->tags.size(); i++) {
      if (way->tags[i].key==tagRef) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->type);

        if (style==NULL ||
            magnification<style->GetMinMag() ||
            style->GetStyle()==LabelStyle::contour) {
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

        xmin=(xmin*gradtorad-hmin)*hscale;
        xmax=(xmax*gradtorad-hmin)*hscale;

        ymin=height-(atanh(sin(ymin*gradtorad))-vmin)*vscale;
        ymax=height-(atanh(sin(ymax*gradtorad))-vmin)*vscale;

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
                  *style,
                  way->tags[i].value,
                  x,y);

        labelMap.insert(tile);
      }
      else if (way->tags[i].key==tagName) {
        const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->type);

        if (style==NULL ||
            magnification<style->GetMinMag() ||
            style->GetStyle()==LabelStyle::contour) {
          continue;
        }

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

        xmin=(xmin*gradtorad-hmin)*hscale;
        xmax=(xmax*gradtorad-hmin)*hscale;

        ymin=height-(atanh(sin(ymin*gradtorad))-vmin)*vscale;
        ymax=height-(atanh(sin(ymax*gradtorad))-vmin)*vscale;

        DrawLabel(draw,
                  *style,
                  way->tags[i].value,
                  xmin+(xmax-xmin)/2,
                  ymin+(ymax-ymin)/2);
      }
    }
  }
  cairo_restore(draw);

  // Nodes

  //std::cout << "Draw nodes..." << std::endl;

  cairo_save(draw);
  for (std::list<Node>::const_iterator node=nodes.begin();
       node!=nodes.end();
       ++node) {
    const SymbolStyle *style=styleConfig.GetNodeSymbolStyle(node->type);

    if (style==NULL || magnification<style->GetMinMag()) {
      continue;
    }

    double x,y;

    x=(node->lon*gradtorad-hmin)*hscale;
    y=height-(atanh(sin(node->lat*gradtorad))-vmin)*vscale;

    DrawSymbol(draw,style,x,y);
  }

  nodesAllCount+=nodes.size();

  cairo_restore(draw);

  // Node labels

  //std::cout << "Draw node labels..." << std::endl;

  cairo_save(draw);
  for (std::list<Node>::const_iterator node=nodes.begin();
       node!=nodes.end();
       ++node) {
    for (size_t i=0; i<node->tags.size(); i++) {
      // TODO: We should make sure we prefer one over the other
      if (node->tags[i].key==tagName) {
        const LabelStyle *style=styleConfig.GetNodeLabelStyle(node->type);

        if (style==NULL || magnification<style->GetMinMag()) {
          continue;
        }

        double x,y;

        x=(node->lon*gradtorad-hmin)*hscale;
        y=height-(atanh(sin(node->lat*gradtorad))-vmin)*vscale;

        DrawLabel(draw,*style,
                  node->tags[i].value,
                  x,y);
      }
      else if (node->tags[i].key==tagRef)  {
        const LabelStyle *style=styleConfig.GetNodeRefLabelStyle(node->type);

        if (style==NULL || magnification<style->GetMinMag()) {
          continue;
        }

        double x,y;

        x=(node->lon*gradtorad-hmin)*hscale;
        y=height-(atanh(sin(node->lat*gradtorad))-vmin)*vscale;

        DrawLabel(draw,*style,
                  node->tags[i].value,
                  x,y);
      }
    }
  }
  cairo_restore(draw);


  // Area labels

  //std::cout << "Draw area labels..." << std::endl;

  cairo_save(draw);
  for (std::list<Way>::const_iterator area=ways.begin();
       area!=ways.end();
       ++area) {

    if (!area->IsArea()) {
      continue;
    }

    const LabelStyle *style=styleConfig.GetAreaLabelStyle(area->type);

    if (style==NULL || style->GetMinMag()>magnification) {
      continue;
    }

    for (size_t i=0; i<area->tags.size(); i++) {
      // TODO: We should make sure we prefare one over the other
      if (area->tags[i].key==tagName || area->tags[i].key==tagRef) {
        double xmin=area->nodes[0].lon;
        double xmax=area->nodes[0].lon;
        double ymin=area->nodes[0].lat;
        double ymax=area->nodes[0].lat;

        for (size_t j=1; j<area->nodes.size(); j++) {
          xmin=std::min(xmin,area->nodes[j].lon);
          xmax=std::max(xmax,area->nodes[j].lon);
          ymin=std::min(ymin,area->nodes[j].lat);
          ymax=std::max(ymax,area->nodes[j].lat);
        }

        xmin=(xmin*gradtorad-hmin)*hscale;
        xmax=(xmax*gradtorad-hmin)*hscale;

        ymin=height-(atanh(sin(ymin*gradtorad))-vmin)*vscale;
        ymax=height-(atanh(sin(ymax*gradtorad))-vmin)*vscale;

        DrawLabel(draw,*style,
                  area->tags[i].value,
                  xmin+(xmax-xmin)/2,
                  ymin+(ymax-ymin)/2);
        break;
      }
    }
  }
  cairo_restore(draw);

  std::cout << "Nodes drawn: " << nodesDrawnCount << std::endl;
  std::cout << "Nodes out: " << nodesOut << std::endl;
  std::cout << "Nodes all: " << nodesAllCount << std::endl;
  std::cout << "Drawing (done)." << std::endl;

  return true;
}

bool MapPainter::PrintMap(const StyleConfig& styleConfig,
                          double lon, double lat,
                          double magnification,
                          size_t width, size_t height)
{
  cairo_surface_t *image=cairo_image_surface_create(CAIRO_FORMAT_RGB24,width,height);
  //cairo_surface_t *image=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);
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

