/*
  This source is part of the libosmscout-map library
  Copyright (C) 2013  Tim Teulings
  Copyright (C) 2017  Fanny Monori

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

#include <utility>
#include <GL/glew.h>
#include <osmscout/MapPainter.h>
#include <osmscout/MapPainterOpenGL.h>
#include <osmscout/Triangulate.h>
#include <iostream>

namespace osmscout {

  osmscout::MapPainterOpenGL::MapPainterOpenGL(int width, int height, int screenWidth, int screenHeight) : width(width),
                                                                                                           height(height),
                                                                                                           screenWidth(
                                                                                                                   screenWidth),
                                                                                                           screenHeight(
                                                                                                                   screenHeight),
                                                                                                           minLon(0),
                                                                                                           minLat(0),
                                                                                                           maxLon(0),
                                                                                                           maxLat(0),
                                                                                                           lookX(0.0),
                                                                                                           lookY(0.0){
    glewExperimental = GL_TRUE;
    glewInit();

    AreaRenderer.LoadVertexShader("AreaVertexShader.vert");
    AreaRenderer.LoadFragmentShader("AreaFragmentShader.frag");
    bool success = AreaRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }

    GroundTileRenderer.LoadVertexShader("GroundVertexShader.vert");
    GroundTileRenderer.LoadFragmentShader("GroundFragmentShader.frag");
    success = GroundTileRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }

    GroundRenderer.LoadVertexShader("GroundVertexShader.vert");
    GroundRenderer.LoadFragmentShader("GroundFragmentShader.frag");
    success = GroundRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }

    PathRenderer.LoadVertexShader("PathVertexShader.vert");
    PathRenderer.LoadFragmentShader("PathFragmentShader.frag");
    success = PathRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }

    AreaRenderer.clearData();
    AreaRenderer.SetVerticesSize(5);
    GroundTileRenderer.clearData();
    GroundTileRenderer.SetVerticesSize(5);
    GroundRenderer.clearData();
    GroundRenderer.SetVerticesSize(5);
    PathRenderer.clearData();
    PathRenderer.SetVerticesSize(11);
  }

  void osmscout::MapPainterOpenGL::loadData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                            const osmscout::Projection &projection,
                                            const osmscout::StyleConfigRef &styleConfig,
                                            const osmscout::GeoBox &BoundingBox) {
    styleConfig.get()->GetLandFillStyle(projection, landFill);
    styleConfig.get()->GetSeaFillStyle(projection, seaFill);
    if (minLat == 0)
      minLat = BoundingBox.GetMinLat();
    if (minLon == 0)
      minLon = BoundingBox.GetMinLon();
    if (maxLat == 0)
      maxLat = BoundingBox.GetMaxLat();
    if (maxLon == 0)
      maxLon = BoundingBox.GetMaxLon();

    this->BoundingBox = BoundingBox;
    this->Magnification = projection.GetMagnification();
    this->Center = projection.GetCenter();

    ProcessAreaData(data, parameter, projection, styleConfig);

    ProcessGroundData(data, parameter, projection, styleConfig);

    ProcessPathData(data, parameter, projection, styleConfig);
  }

  void osmscout::MapPainterOpenGL::SwapData() {

    AreaRenderer.SwapData();

    AreaRenderer.BindBuffers();
    AreaRenderer.LoadProgram();
    AreaRenderer.LoadVertices();

    AreaRenderer.SetProjection(width, height);
    AreaRenderer.SetModel();
    AreaRenderer.SetView(lookX, lookY);
    AreaRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    AreaRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    AreaRenderer.AddUniform("minLon", minLon);
    AreaRenderer.AddUniform("minLat", minLat);
    AreaRenderer.AddUniform("maxLon", maxLon);
    AreaRenderer.AddUniform("maxLat", maxLat);
    AreaRenderer.AddUniform("windowWidth", width);
    AreaRenderer.AddUniform("windowHeight", height);
    AreaRenderer.AddUniform("centerLat", Center.GetLat());
    AreaRenderer.AddUniform("centerLon", Center.GetLon());
    AreaRenderer.AddUniform("magnification", Magnification.GetMagnification());


    GroundTileRenderer.SwapData();

    GroundTileRenderer.BindBuffers();
    GroundTileRenderer.LoadProgram();
    GroundTileRenderer.LoadVertices();

    GroundTileRenderer.SetProjection(width, height);
    GroundTileRenderer.SetModel();
    GroundTileRenderer.SetView(lookX, lookY);
    GroundTileRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    GroundTileRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    GroundTileRenderer.AddUniform("minLon", minLon);
    GroundTileRenderer.AddUniform("minLat", minLat);
    GroundTileRenderer.AddUniform("maxLon", maxLon);
    GroundTileRenderer.AddUniform("maxLat", maxLat);
    GroundTileRenderer.AddUniform("windowWidth", width);
    GroundTileRenderer.AddUniform("windowHeight", height);

    GroundRenderer.SwapData();

    GroundRenderer.BindBuffers();

    GroundRenderer.LoadProgram();
    GroundRenderer.LoadVertices();

    GroundRenderer.SetProjection(width, height);
    GroundRenderer.SetModel();
    GroundRenderer.SetView(lookX, lookY);
    GroundRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    GroundRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    GroundRenderer.AddUniform("minLon", minLon);
    GroundRenderer.AddUniform("minLat", minLat);
    GroundRenderer.AddUniform("maxLon", maxLon);
    GroundRenderer.AddUniform("maxLat", maxLat);

    PathRenderer.SwapData();

    PathRenderer.BindBuffers();
    PathRenderer.LoadProgram();
    PathRenderer.LoadVertices();

    PathRenderer.SetProjection(width, height);
    PathRenderer.SetModel();
    PathRenderer.SetView(lookX, lookY);
    PathRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    PathRenderer.AddAttrib("previous", 2, GL_FLOAT, 2 * sizeof(GLfloat));
    PathRenderer.AddAttrib("next", 2, GL_FLOAT, 4 * sizeof(GLfloat));
    PathRenderer.AddAttrib("color", 3, GL_FLOAT, 6 * sizeof(GLfloat));
    PathRenderer.AddAttrib("index", 1, GL_FLOAT, 9 * sizeof(GLfloat));
    PathRenderer.AddAttrib("thickness", 1, GL_FLOAT, 10 * sizeof(GLfloat));
    PathRenderer.AddUniform("minLon", minLon);
    PathRenderer.AddUniform("minLat", minLat);
    PathRenderer.AddUniform("maxLon", maxLon);
    PathRenderer.AddUniform("maxLat", maxLat);
    PathRenderer.AddUniform("windowWidth", width);
    PathRenderer.AddUniform("windowHeight", height);
    PathRenderer.AddUniform("centerLat", Center.GetLat());
    PathRenderer.AddUniform("centerLon", Center.GetLon());
    PathRenderer.AddUniform("magnification", Magnification.GetMagnification());
  }

  void
  osmscout::MapPainterOpenGL::ProcessAreaData(const osmscout::MapData &data,
                                              const osmscout::MapParameter &parameter,
                                              const osmscout::Projection &projection,
                                              const osmscout::StyleConfigRef &styleConfig) {

    for (const auto &area : data.areas) {

      size_t ringId = Area::outerRingId;
      bool foundRing = true;

      while (foundRing) {
        foundRing = false;

        for (size_t i = 0; i < area->rings.size(); i++) {
          const Area::Ring &ring = area->rings[i];

          if (ring.IsMasterRing()) {
            continue;
          }

          if (ring.GetRing() != ringId) {
            continue;
          }

          if (!ring.IsOuterRing() &&
              ring.GetType()->GetIgnore()) {
            continue;
          }

          if (!ring.IsOuterRing() && ring.GetType()->GetIgnore())
            continue;

          TypeInfoRef type;
          FillStyleRef fillStyle;
          std::vector<BorderStyleRef> borderStyles;
          BorderStyleRef borderStyle;

          if (ring.IsOuterRing()) {
            type = area->GetType();
          } else {
            type = ring.GetType();
          }

          styleConfig->GetAreaFillStyle(type,
                                        ring.GetFeatureValueBuffer(),
                                        projection,
                                        fillStyle);

          styleConfig->GetAreaBorderStyles(type,
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           borderStyles);

          if (!fillStyle && borderStyles.empty()) {
            continue;
          }

          foundRing = true;

          std::vector<Point> p = area->rings[i].nodes;
          std::vector<osmscout::Area::Ring> r;

          for (int i = p.size() - 1; i >= 0; i--) {
            osmscout::GeoCoord g = osmscout::GeoCoord(ring.nodes[i].GetLat(), ring.nodes[i].GetLon());
            //std::cout << "geo1: " << ring.nodes[i].GetLon() << " " << ring.nodes[i].GetLat() << std::endl;
            glm::vec2 pos = GeoToOpenGLPixel(g);
            for (int j = 0; j < i; j++) {
              if (fabs(p[i].GetLat() - p[j].GetLat()) < 0.000000001 &&
                  fabs(p[i].GetLon() - p[j].GetLon()) < 0.0000000001) {
                p.erase(p.begin() + i);
              }
            }
          }

          if (p.size() < 3)
            continue;

          size_t j = i + 1;
          int hasClippings = 0;
          while (j < area->rings.size() &&
                 area->rings[j].GetRing() == ringId + 1 &&
                 area->rings[j].GetType()->GetIgnore()) {
            r.push_back(area->rings[j]);
            j++;
            hasClippings = 1;
          }

          //border TODO

          Color c = fillStyle->GetFillColor();

          std::vector<GLfloat> points;
          if (hasClippings == 1) {
            for (auto &ring: r) {
              for (int i = ring.nodes.size() - 1; i >= 0; i--) {
                for (int j = 0; j < i; j++) {
                  if (fabs(ring.nodes[i].GetLat() - ring.nodes[j].GetLat()) < 0.000000001 &&
                      fabs(ring.nodes[i].GetLon() - ring.nodes[j].GetLon()) < 0.0000000001) {
                    ring.nodes.erase(ring.nodes.begin() + i);
                  }
                }
              }
            }

            std::vector<std::vector<osmscout::Point>> polygons;
            polygons.push_back(p);
            for (const auto &ring: r) {
              if (ring.nodes.size() >= 3)
                polygons.push_back(ring.nodes);
            }
            points = osmscout::Triangulate::TriangulateWithHoles(polygons);
          } else {

            points = osmscout::Triangulate::TriangulatePolygon(p);
          }

          for (size_t t = 0; t < points.size(); t++) {
            if (t % 2 == 0) {
              AreaRenderer.AddNewVertex(points[t]);
            } else {
              AreaRenderer.AddNewVertex(points[t]);
              AreaRenderer.AddNewVertex(c.GetR());
              AreaRenderer.AddNewVertex(c.GetG());
              AreaRenderer.AddNewVertex(c.GetB());

              if (AreaRenderer.GetNumOfVertices() <= 5) {
                AreaRenderer.AddNewElement(0);
              } else {
                AreaRenderer.AddNewElement(AreaRenderer.GetVerticesNumber() - 1);
              }

            }
          }

        }
        ringId++;
      }
    }
  }

  void
  osmscout::MapPainterOpenGL::ProcessPathData(const osmscout::MapData &data,
                                              const osmscout::MapParameter &/*parameter*/,
                                              const osmscout::Projection &projection,
                                              const osmscout::StyleConfigRef &styleConfig) {

    WidthFeatureValueReader widthReader(*styleConfig->GetTypeConfig());
    LayerFeatureValueReader layerReader(*styleConfig->GetTypeConfig());

    for (const auto &way: data.ways) {
      styleConfig->GetWayLineStyles(way->GetFeatureValueBuffer(),
                                    projection,
                                    lineStyles);

      if (lineStyles.empty()) {
        return;
      }

      FeatureValueBuffer buffer(way->GetFeatureValueBuffer());

      for (const auto &lineStyle : lineStyles) {
        double lineWidth = 0.0;
        double lineOffset = 0.0;

        /*if (lineStyle->GetWidth() > 0.0) {
          WidthFeatureValue *widthValue = widthReader.GetValue(buffer);

          std::cout << "width " << widthValue << " " << lineStyle->GetWidth() << std::endl;

          if (widthValue != NULL) {
            lineWidth += widthValue->GetWidth() / projection.GetPixelSize();
          } else {
            lineWidth += lineStyle->GetWidth() / projection.GetPixelSize();
          }

          std::cout << "width2 " << lineWidth << std::endl;
        }*/

        if (lineStyle->GetDisplayWidth() > 0.0) {
          lineWidth += projection.ConvertWidthToPixel(lineStyle->GetDisplayWidth());
        }

        if (lineWidth == 0.0) {
          continue;
        }

        if (lineStyle->GetOffset() != 0.0) {
          lineOffset += lineStyle->GetOffset() / projection.GetPixelSize();
        }

        if (lineStyle->GetDisplayOffset() != 0.0) {
          lineOffset += projection.ConvertWidthToPixel(lineStyle->GetDisplayOffset());
        }

        //if(lineWidth < 1)
        //  continue;
        //std::cout << lineWidth << " " << lineOffset << " " << lineStyle->GetDisplayWidth() << " " << lineStyle->GetWidth() << std::endl;

        for (size_t i = 0; i < way->nodes.size() - 1; i++) {
          PathRenderer.AddNewVertex(way->nodes[i].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i].GetLat());
          if (i == 0) {
            PathRenderer.AddNewVertex(way->nodes[i].GetLon());
            PathRenderer.AddNewVertex(way->nodes[i].GetLat());
          } else {
            PathRenderer.AddNewVertex(way->nodes[i - 1].GetLon());
            PathRenderer.AddNewVertex(way->nodes[i - 1].GetLat());
          }
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLat());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetR());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetG());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetB());
          if (i == 0)
            PathRenderer.AddNewVertex(1.0);
          else
            PathRenderer.AddNewVertex(5.0);
          PathRenderer.AddNewVertex(lineWidth);

          PathRenderer.AddNewVertex(way->nodes[i].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i].GetLat());
          if (i == 0) {
            PathRenderer.AddNewVertex(way->nodes[i].GetLon());
            PathRenderer.AddNewVertex(way->nodes[i].GetLat());
          } else {
            PathRenderer.AddNewVertex(way->nodes[i - 1].GetLon());
            PathRenderer.AddNewVertex(way->nodes[i - 1].GetLat());
          }
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLat());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetR());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetG());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetB());
          if (i == 0)
            PathRenderer.AddNewVertex(2.0);
          else
            PathRenderer.AddNewVertex(6.0);
          PathRenderer.AddNewVertex(lineWidth);

          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLat());
          PathRenderer.AddNewVertex(way->nodes[i].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i].GetLat());
          PathRenderer.AddNewVertex(way->nodes[i + 2].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i + 2].GetLat());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetR());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetG());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetB());
          if (i == way->nodes.size() - 2)
            PathRenderer.AddNewVertex(7.0);
          else
            PathRenderer.AddNewVertex(3.0);
          PathRenderer.AddNewVertex(lineWidth);
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i + 1].GetLat());
          PathRenderer.AddNewVertex(way->nodes[i].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i].GetLat());
          PathRenderer.AddNewVertex(way->nodes[i + 2].GetLon());
          PathRenderer.AddNewVertex(way->nodes[i + 2].GetLat());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetR());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetG());
          PathRenderer.AddNewVertex(lineStyle->GetLineColor().GetB());
          if (i == way->nodes.size() - 2)
            PathRenderer.AddNewVertex(8.0);
          else
            PathRenderer.AddNewVertex(4.0);
          PathRenderer.AddNewVertex(lineWidth);

          int num;
          if (PathRenderer.GetNumOfVertices() <= 44) {
            num = 0;
          } else {
            num = PathRenderer.GetVerticesNumber() - 4;
          }
          PathRenderer.AddNewElement(num);
          PathRenderer.AddNewElement(num + 1);
          PathRenderer.AddNewElement(num + 2);
          PathRenderer.AddNewElement(num + 2);
          PathRenderer.AddNewElement(num + 1);
          PathRenderer.AddNewElement(num + 3);
        }
      }
    }
  }

  void
  osmscout::MapPainterOpenGL::ProcessGroundData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                                const osmscout::Projection &projection,
                                                const osmscout::StyleConfigRef &styleConfig) {
    FillStyleRef landFill;

    styleConfig->GetLandFillStyle(projection,
                                  landFill);

    if (!landFill) {
      landFill = this->landFill;
    }

    FillStyleRef seaFill;
    FillStyleRef coastFill;
    FillStyleRef unknownFill;
    std::vector<Point> points;

    styleConfig->GetSeaFillStyle(projection,
                                 seaFill);
    styleConfig->GetCoastFillStyle(projection,
                                   coastFill);
    styleConfig->GetUnknownFillStyle(projection,
                                     unknownFill);

    if (!seaFill) {
      seaFill = this->seaFill;
    }

    for (const auto &tile : data.groundTiles) {
      if (tile.type == GroundTile::unknown &&
          !parameter.GetRenderUnknowns()) {
        continue;
      }

      FillStyleRef fill;

      switch (tile.type) {
        case GroundTile::land:
          fill = landFill;
          break;
        case GroundTile::water:
          fill = seaFill;
          break;
        case GroundTile::coast:
          fill = seaFill;
          break;
        case GroundTile::unknown:
          fill = unknownFill;
          break;
      }

      GeoCoord minCoord(tile.yAbs * tile.cellHeight - 90.0,
                        tile.xAbs * tile.cellWidth - 180.0);
      GeoCoord maxCoord(minCoord.GetLat() + tile.cellHeight,
                        minCoord.GetLon() + tile.cellWidth);

      if (tile.coords.empty()) {
        GroundTileRenderer.AddNewVertex(minCoord.GetLon());
        GroundTileRenderer.AddNewVertex(minCoord.GetLat());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        GroundTileRenderer.AddNewVertex(maxCoord.GetLon());
        GroundTileRenderer.AddNewVertex(minCoord.GetLat());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        GroundTileRenderer.AddNewVertex(maxCoord.GetLon());
        GroundTileRenderer.AddNewVertex(maxCoord.GetLat());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());


        GroundTileRenderer.AddNewVertex(minCoord.GetLon());
        GroundTileRenderer.AddNewVertex(minCoord.GetLat());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        GroundTileRenderer.AddNewVertex(minCoord.GetLon());
        GroundTileRenderer.AddNewVertex(maxCoord.GetLat());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        GroundTileRenderer.AddNewVertex(maxCoord.GetLon());
        GroundTileRenderer.AddNewVertex(maxCoord.GetLat());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        GroundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        int num;
        if (GroundTileRenderer.GetVerticesNumber() <= 6)
          num = 0;
        else
          num = GroundTileRenderer.GetVerticesNumber();
        for (size_t i = 0; i < 6; i++)
          GroundTileRenderer.AddNewElement(num + i);

      } else {

        std::vector<osmscout::Point> p;
        for (size_t i = 0; i < tile.coords.size(); i++) {
          double lat;
          double lon;
          lat = minCoord.GetLat() + tile.coords[i].y * tile.cellHeight / GroundTile::Coord::CELL_MAX;
          lon = minCoord.GetLon() + tile.coords[i].x * tile.cellWidth / GroundTile::Coord::CELL_MAX;

          osmscout::GeoCoord g = osmscout::GeoCoord(lat, lon);
          osmscout::Point pt;
          pt.SetCoord(g);
          p.push_back(pt);

        }

        for (int i = p.size() - 1; i >= 0; i--) {
          for (int j = 0; j < i; j++) {
            if (fabs(p[i].GetLat() - p[j].GetLat()) < 0.000000001 &&
                fabs(p[i].GetLon() - p[j].GetLon()) < 0.0000000001) {
              p.erase(p.begin() + i);
            }
          }
        }

        std::vector<GLfloat> points;

        points = osmscout::Triangulate::TriangulatePolygon(p);

        for (size_t t = 0; t < points.size(); t++) {
          if (t % 2 == 0) {
            GroundRenderer.AddNewVertex(points[t]);
          } else {
            GroundRenderer.AddNewVertex(points[t]);
            GroundRenderer.AddNewVertex(fill->GetFillColor().GetR());
            GroundRenderer.AddNewVertex(fill->GetFillColor().GetG());
            GroundRenderer.AddNewVertex(fill->GetFillColor().GetB());

            if (GroundRenderer.GetNumOfVertices() <= 5) {
              GroundRenderer.AddNewElement(0);
            } else {
              GroundRenderer.AddNewElement(GroundRenderer.GetVerticesNumber() - 1);
            }

          }
        }

      }

    }
  }

  void osmscout::MapPainterOpenGL::onZoom(float zoom, float zoomScale) {
    /*float l = pow(2.0,zoomScale * zoom);
    std::cout << "l: " << l << std::endl;*/
    /*AreaRenderer.ScaleModel(zoomScale * zoom);
    GroundRenderer.ScaleModel(zoomScale * zoom);
    GroundTileRenderer.ScaleModel(zoomScale * zoom);
    PathRenderer.ScaleModel(zoomScale * zoom);*/
    AreaRenderer.ScaleModel(zoomScale * zoom);
    GroundRenderer.ScaleModel(zoomScale * zoom);
    GroundTileRenderer.ScaleModel(zoomScale * zoom);
    PathRenderer.ScaleModel(zoomScale * zoom);

    /*lookX += (zoomScale * zoom);
    lookY += (zoomScale * zoom);

    AreaRenderer.SetView(lookX, lookY);
    GroundTileRenderer.SetView(lookX, lookY);
    GroundRenderer.SetView(lookX, lookY);
    PathRenderer.SetView(lookX, lookY);*/

  }

  void osmscout::MapPainterOpenGL::ZoomTo(float zoom, float zoomScale, float x, float y){
    AreaRenderer.ScaleModel(zoomScale * zoom);
    GroundRenderer.ScaleModel(zoomScale * zoom);
    GroundTileRenderer.ScaleModel(zoomScale * zoom);
    PathRenderer.ScaleModel(zoomScale * zoom);

    double OffsetX = x - (width/2);
    double OffsetY = y - (height/2);

    /*this->minLon += OffsetX / 100000;
    this->minLat += OffsetY / 100000;
    this->maxLon += OffsetX / 100000;
    this->maxLat += OffsetY / 100000;*/

    lookX += OffsetX / 10000;
    lookY += OffsetY / 10000;

    AreaRenderer.SetView(lookX, lookY);
    GroundTileRenderer.SetView(lookX, lookY);
    GroundRenderer.SetView(lookX, lookY);
    PathRenderer.SetView(lookX, lookY);

  }

  void osmscout::MapPainterOpenGL::SetCenter(osmscout::GeoCoord){

  }

  void osmscout::MapPainterOpenGL::onTranslation(int startPointX, int startPointY, int endPointX, int endPointY) {
    float offsetX = startPointX - endPointX;
    float offsetY = endPointY - startPointY;

    /*lookX += offsetX / 1000;
    lookY += offsetY / 1000;*/

    this->minLon += offsetX / 500;
    this->minLat += offsetY / 500;
    this->maxLon += offsetX / 500;
    this->maxLat += offsetY / 500;

    GeoCoord g = osmscout::GeoCoord(Center.GetLat() + offsetY / 500, Center.GetLon() + offsetX / 500);
    Center = g;

    /*AreaRenderer.SetView(lookX, lookY);
    GroundTileRenderer.SetView(lookX, lookY);
    GroundRenderer.SetView(lookX, lookY);
    PathRenderer.SetView(lookX, lookY);*/
  }


  float latOffset;

  glm::vec4
      osmscout::MapPainterOpenGL::GeoToOpenGLPixel(osmscout::GeoCoord gc) {
    float PI = 3.1415926535897;
    float R = 6378137.0;

    /*float deg_rad = 180 / PI;
    float y1 = std::log(tan((gc.GetLat() / deg_rad) / 2 + PI / 4));
    float merc_y = y1 * deg_rad;

    //std::cout << "Merc: " << gc.GetLon() << " " << merc_y <<  std::endl;

    float minLat_m = (std::log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (std::log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    float height = abs(minLat_m - maxLat_m);
    float width = abs(minLon - maxLon);
    float x_width = width/height;
    float y_height = 1;

    float x = ((2*x_width)*(gc.GetLon() - (minLon))/((maxLon)-(minLon)))-x_width;
    float y = ((2*y_height)*(merc_y - (minLat_m))/((maxLat_m)-(minLat_m)))-y_height;

    glm::vec4 lel = glm::vec4(x,y,0.0,1.0);
    glm::mat4x4 mvp =  GroundRenderer.GetProjection() * GroundRenderer.GetView() * GroundRenderer.GetModel();
    glm::vec4 pixelPos = mvp * lel;*/

    /*float latMin=std::max(MinLat,std::min(std::min(tlLat,trLat),std::min(blLat,brLat)));
    float latMax=std::min(MaxLat,std::max(std::max(tlLat,trLat),std::max(blLat,brLat)));

    float lonMin=std::max(MinLon,std::min(std::min(tlLon,trLon),std::min(blLon,brLon)));
    float lonMax=std::min(MaxLon,std::max(std::max(tlLon,trLon),std::max(blLon,brLon)));*/

    // Resulting projection scale factor

    double tlLat;
    double tlLon;

    PixelToGeoOrig(0.0,0.0,tlLon,tlLat);

    // top right
    double trLat;
    double trLon;

    PixelToGeoOrig((double)width,0.0,trLon,trLat);

    // bottom left
    double blLat;
    double blLon;

    PixelToGeoOrig(0.0,(double)height,blLon,blLat);

    // bottom right
    double brLat;
    double brLon;

    PixelToGeoOrig((double)width,(double)height,brLon,brLat);

    double MaxLat = +85.0511;
    double MinLat = -85.0511;
    double MaxLon = +180.0;
    double MinLon = -180.0;

    float latMin=std::max(MinLat,std::min(std::min(tlLat,trLat),std::min(blLat,brLat)));
    float latMax=std::min(MaxLat,std::max(std::max(tlLat,trLat),std::max(blLat,brLat)));

    float lonMin=std::max(MinLon,std::min(std::min(tlLon,trLon),std::min(blLon,brLon)));
    float lonMax=std::min(MaxLon,std::max(std::max(tlLon,trLon),std::max(blLon,brLon)));

    std::cout << lonMin << " " << latMin << " - " << lonMax << " " << latMax << std::endl;

    float tileDPI=96.0;
    float gradtorad=2*M_PI/360;
    float earthRadiusMeter=6378137.0;
    float earthExtentMeter=2*M_PI*earthRadiusMeter;
    float tileWidthZoom0Aquator=earthExtentMeter;
    float equatorTileWidth=tileWidthZoom0Aquator/Magnification.GetMagnification();
    float equatorTileResolution=equatorTileWidth/256.0;
    float equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/96;
    float groundWidthEquatorMeter=width*equatorCorrectedEquatorTileResolution;
    float groundWidthVisibleMeter=groundWidthEquatorMeter*cos(gc.GetLat()*gradtorad);

    float scale=width/(2*M_PI*groundWidthEquatorMeter/earthExtentMeter);
    float scaleGradtorad=scale*gradtorad;

    //float lonOffset=lonMin*scaleGradtorad;
    latOffset=atanh(sin(Center.GetLat()*gradtorad));

    double latDeriv = 1.0 / sin( (2 * Center.GetLat() * gradtorad + M_PI) /  2);
    float scaledLatDeriv = latDeriv * gradtorad * scale;

    float x2=(gc.GetLon()-Center.GetLon())*scaledLatDeriv;


    //float y2=(atanh(sin(gc.GetLat()*gradtorad)))*scale;
    float y2=(atanh(sin(gc.GetLat()*gradtorad))-latOffset)*scale;
    //float y2 = (gc.GetLat()-Center.GetLat())*scaledLatDeriv;;

    std::cout << "gradtorad: " << gradtorad << std::endl;
    std::cout << "y " << y2 << " " << latOffset  << std::endl;

    y2=height/2-y2;
    x2 += width/2;

    float deg_rad = 180 / PI;
    float minLat_m = (std::log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (std::log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    /*float height = abs(minLat_m - maxLat_m);
    float width = abs(minLon - maxLon);
    float x_width = width/height;
    float y_height = 1;

    float x3 = ((2*x_width)*(x2)/((width)))-x_width;
    float y3 = ((2*y_height)*(y2)/((height)))-y_height;*/

    //glm::vec4 pixelP = glm::vec4(x3, y3, 0.0, 1.0);
    glm::vec4 pixelPos = AreaRenderer.GetProjection() * AreaRenderer.GetView() * AreaRenderer.GetModel() * pixelPos;

    std::cout << "screen: " << x2 << " " << y2 << " " << glm::to_string(pixelPos) << std::endl;

    return pixelPos;

  }

  bool osmscout::MapPainterOpenGL::PixelToGeoOrig(double x, double y,
                                      double& lon, double& lat)
  {

    /*if (angle!=0.0) {
      double xn=x*angleCos-y*angleSin;
      double yn=x*angleSin+y*angleCos;

      x=xn;
      y=yn;
    }*/

    float tileDPI=96.0;
    float gradtorad=2*M_PI/360;
    float earthRadiusMeter=6378137.0;
    float earthExtentMeter=2*M_PI*earthRadiusMeter;
    float tileWidthZoom0Aquator=earthExtentMeter;
    float equatorTileWidth=tileWidthZoom0Aquator/Magnification.GetMagnification();
    float equatorTileResolution=equatorTileWidth/256.0;
    float equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/96;
    float groundWidthEquatorMeter=width*equatorCorrectedEquatorTileResolution;
    //float groundWidthVisibleMeter=groundWidthEquatorMeter*cos(gc.GetLat()*gradtorad);

    float scale=width/(2*M_PI*groundWidthEquatorMeter/earthExtentMeter);
    float scaleGradtorad=scale*gradtorad;

    x-=width/2;
    y=height/2-y;

    // Transform to absolute geo coordinate
    lon=Center.GetLon()+x/scaleGradtorad;
    lat=atan(sinh(y/scale+latOffset))/gradtorad;

    return true;
  }


  /*glm::vec2
  osmscout::MapPainterOpenGL::GeoToPixel(osmscout::GeoCoord gc) {



    return;
  }*/

  osmscout::GeoCoord
  osmscout::MapPainterOpenGL::PixelToGeo(glm::vec4 pixel) {
    float PI = 3.1415926535897;
    float R = 6378137.0;

   // std::cout << "p: " << pixel.x << " " << pixel.y << std::endl;

    float deg_rad = 180 / PI;
    float minLat_m = (std::log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (std::log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    float height = abs(minLat_m - maxLat_m);
    float width = abs(minLon - maxLon);
    float x_width = width/height;
    float y_height = 1;

    float openlgx = ((2*x_width)*(pixel.x)/((this->width)))-x_width;
    float opengly = ((2*y_height)*(pixel.y)/((this->height)))-y_height;

    std::cout << "op: " << openlgx << " " << opengly << std::endl;

    glm::vec4 p = glm::vec4(openlgx,opengly, 0.0, 1.0);
    return OpenGLPixelToGeo(p);
  }

  /*osmscout::MapPainterOpenGL::PixelToGeo(glm::vec4 pixel) {
  float PI = 3.1415926535897;
  float R = 6378137.0;

  // std::cout << "p: " << pixel.x << " " << pixel.y << std::endl;

  float deg_rad = 180 / PI;
  float minLat_m = (std::log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
  float maxLat_m = (std::log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

  float height = abs(minLat_m - maxLat_m);
  float width = abs(minLon - maxLon);
  float x_width = width/height;
  float y_height = 1;

  float openlgx = ((2*x_width)*(pixel.x)/((this->width)))-x_width;
  float opengly = ((2*y_height)*(pixel.y)/((this->height)))-y_height;

  // std::cout << "op: " << openlgx << " " << opengly << std::endl;

  glm::vec4 p = glm::vec4(openlgx,opengly, 0.0, 1.0);
  return p;
}*/

  osmscout::GeoCoord
  osmscout::MapPainterOpenGL::OpenGLPixelToGeo(glm::vec4 pixel) {

    float PI = 3.1415926535897;
    float R = 6378137.0;

    float deg_rad = 180 / PI;
    float minLat_m = (std::log(tan((minLat / deg_rad) / 2 + PI / 4))) * deg_rad;
    float maxLat_m = (std::log(tan((maxLat / deg_rad) / 2 + PI / 4))) * deg_rad;

    float x_width = this->width/this->height;
    float minx = -1 * x_width / (float)2;
    float miny = -1;
    float maxx = x_width / (float)2;
    float maxy = 1;
    float lonwidth = maxLon - minLon;
    float latheight = maxLat_m - minLat_m;

    glm::mat4x4 inv = glm::inverse(( GroundRenderer.GetProjection() * GroundRenderer.GetModel() * GroundRenderer.GetView()));
    glm::vec4 pixelPos = inv * pixel;
    //glm::vec3 pixelPos = glm::project(lel, GroundRenderer.GetModel() , GroundRenderer.GetProjection(), viewport)
    //std::cout << "before: " << glm::to_string(pixel) << " after: " << glm::to_string(pixelPos) << std::endl;

    float OldRangeX = (maxx - minx);
    float NewRangeX = (maxLon - minLon);
    float OldRangeY = (maxy - miny);
    float NewRangeY = (maxLat_m - minLat_m);
    float x = (((pixelPos.x - minx) * NewRangeX) / OldRangeX) + minLon;
    float y = (((pixelPos.y - miny) * NewRangeY) / OldRangeY) + minLat_m;

    //std::cout << "lel: " << x << " " << y << std::endl;

    float y1 = ( atan(exp( (y / deg_rad) )) * 2 - M_PI/2 ) * deg_rad;

    //std::cout << "newgeo: " << x << " "  << y1 << std::endl;
    osmscout::GeoCoord g(x, y1);
    return g;
  }

  void osmscout::MapPainterOpenGL::DrawMap() {
    glClearColor(this->landFill.get()->GetFillColor().GetR(),
                 this->landFill.get()->GetFillColor().GetB(),
                 this->landFill.get()->GetFillColor().GetG(),
                 this->landFill.get()->GetFillColor().GetA());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(GroundTileRenderer.getVAO());
    glUseProgram(GroundTileRenderer.getShaderProgram());

    GroundTileRenderer.AddUniform("minLon", minLon);
    GroundTileRenderer.AddUniform("minLat", minLat);
    GroundTileRenderer.AddUniform("maxLon", maxLon);
    GroundTileRenderer.AddUniform("maxLat", maxLat);
    GroundTileRenderer.AddUniform("windowWidth", width);
    GroundTileRenderer.AddUniform("windowHeight", height);
    GroundTileRenderer.AddUniform("centerLat", Center.GetLat());
    GroundTileRenderer.AddUniform("centerLon", Center.GetLon());
    GroundTileRenderer.AddUniform("magnification", Magnification.GetMagnification());

    GroundTileRenderer.SetProjection(width, height);
    GroundTileRenderer.SetModel();
    GroundTileRenderer.SetView(lookX, lookY);
    GroundTileRenderer.Draw();

    glBindVertexArray(GroundRenderer.getVAO());
    glUseProgram(GroundRenderer.getShaderProgram());

    GroundRenderer.AddUniform("minLon", minLon);
    GroundRenderer.AddUniform("minLat", minLat);
    GroundRenderer.AddUniform("maxLon", maxLon);
    GroundRenderer.AddUniform("maxLat", maxLat);
    GroundRenderer.AddUniform("windowWidth", width);
    GroundRenderer.AddUniform("windowHeight", height);
    GroundRenderer.AddUniform("centerLat", Center.GetLat());
    GroundRenderer.AddUniform("centerLon", Center.GetLon());
    GroundRenderer.AddUniform("magnification", Magnification.GetMagnification());

    GroundRenderer.SetProjection(width, height);
    GroundRenderer.SetModel();
    GroundRenderer.SetView(lookX, lookY);
    GroundRenderer.Draw();

    glBindVertexArray(AreaRenderer.getVAO());
    glUseProgram(AreaRenderer.getShaderProgram());

    AreaRenderer.AddUniform("minLon", minLon);
    AreaRenderer.AddUniform("minLat", minLat);
    AreaRenderer.AddUniform("maxLon", maxLon);
    AreaRenderer.AddUniform("maxLat", maxLat);
    AreaRenderer.AddUniform("windowWidth", width);
    AreaRenderer.AddUniform("windowHeight", height);
    AreaRenderer.AddUniform("centerLat", Center.GetLat());
    AreaRenderer.AddUniform("centerLon", Center.GetLon());
    AreaRenderer.AddUniform("magnification", Magnification.GetMagnification());

    AreaRenderer.SetProjection(width, height);
    AreaRenderer.SetModel();
    AreaRenderer.SetView(lookX, lookY);
    AreaRenderer.Draw();

    glBindVertexArray(PathRenderer.getVAO());
    glUseProgram(PathRenderer.getShaderProgram());

    PathRenderer.AddUniform("minLon", minLon);
    PathRenderer.AddUniform("minLat", minLat);
    PathRenderer.AddUniform("maxLon", maxLon);
    PathRenderer.AddUniform("maxLat", maxLat);
    PathRenderer.AddUniform("windowWidth", width);
    PathRenderer.AddUniform("windowHeight", height);
    PathRenderer.AddUniform("centerLat", Center.GetLat());
    PathRenderer.AddUniform("centerLon", Center.GetLon());
    PathRenderer.AddUniform("magnification", Magnification.GetMagnification());

    PathRenderer.SetProjection(width, height);
    PathRenderer.SetModel();
    PathRenderer.SetView(lookX, lookY);
    PathRenderer.Draw();

  }

}
