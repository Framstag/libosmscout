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
                                                                                                           lookY(0.0) {
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
    PathRenderer.AddUniform("screenWidth", screenWidth);
    PathRenderer.AddUniform("screenHeight", screenHeight);
  }

  void
  osmscout::MapPainterOpenGL::ProcessAreaData(const osmscout::MapData &data,
                                              const osmscout::MapParameter &/*parameter*/,
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
  }

  void osmscout::MapPainterOpenGL::onTranslation(int startPointX, int startPointY, int endPointX, int endPointY) {
    float offsetX = startPointX - endPointX;
    float offsetY = endPointY - startPointY;

    lookX += offsetX / 1000;
    lookY += offsetY / 1000;

    AreaRenderer.SetView(lookX, lookY);
    GroundTileRenderer.SetView(lookX, lookY);
    GroundRenderer.SetView(lookX, lookY);
    PathRenderer.SetView(lookX, lookY);
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
    PathRenderer.AddUniform("screenWidth", screenWidth);
    PathRenderer.AddUniform("screenHeight", screenHeight);

    PathRenderer.SetProjection(width, height);
    PathRenderer.SetModel();
    PathRenderer.SetView(lookX, lookY);
    PathRenderer.Draw();

  }

}
