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
#include <osmscout/PNGLoaderOpenGL.h>

namespace osmscout {

  osmscout::MapPainterOpenGL::MapPainterOpenGL(int width, int height, double dpi, int screenWidth, int screenHeight,
                                               std::string fontPath)
      : width(width),
        height(height),
        dpi(dpi),
        screenWidth(
            screenWidth),
        screenHeight(
            screenHeight),
        Textloader(fontPath) {
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

    ImageRenderer.LoadVertexShader("QuadVertexShader.vert");
    ImageRenderer.LoadFragmentShader("QuadFragmentShader.frag");
    success = ImageRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for image rendering!" << std::endl;
      return;
    }

    TextRenderer.LoadVertexShader("TextVertexShader.vert");
    TextRenderer.LoadFragmentShader("TextFragmentShader.frag");
    success = TextRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for text rendering!" << std::endl;
      return;
    }

    AreaRenderer.clearData();
    AreaRenderer.SetVerticesSize(5);
    GroundTileRenderer.clearData();
    GroundTileRenderer.SetVerticesSize(5);
    GroundRenderer.clearData();
    GroundRenderer.SetVerticesSize(5);
    PathRenderer.clearData();
    PathRenderer.SetVerticesSize(14);
    ImageRenderer.clearData();
    ImageRenderer.SetVerticesSize(5);
    ImageRenderer.SetTextureHeight(7);
    TextRenderer.clearData();
    TextRenderer.SetVerticesSize(11);
    TextRenderer.SetTextureHeight(Textloader.GetHeight());
  }

  void osmscout::MapPainterOpenGL::LoadData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                            const osmscout::Projection &projection,
                                            const osmscout::StyleConfigRef &styleConfig) {
    styleConfig.get()->GetLandFillStyle(projection, landFill);
    styleConfig.get()->GetSeaFillStyle(projection, seaFill);

    this->Magnification = projection.GetMagnification();
    this->Center = projection.GetCenter();
    this->Parameter = parameter;

    ProcessAreaData(data, parameter, projection, styleConfig);

    ProcessGroundData(data, parameter, projection, styleConfig);

    ProcessPathData(data, parameter, projection, styleConfig);

    ProcessNodeData(data, parameter, projection, styleConfig);
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

    AreaRenderer.AddUniform("windowWidth", width);
    AreaRenderer.AddUniform("windowHeight", height);
    AreaRenderer.AddUniform("centerLat", Center.GetLat());
    AreaRenderer.AddUniform("centerLon", Center.GetLon());
    AreaRenderer.AddUniform("magnification", Magnification.GetMagnification());
    AreaRenderer.AddUniform("dpi", dpi);

    GroundTileRenderer.SwapData();

    GroundTileRenderer.BindBuffers();
    GroundTileRenderer.LoadProgram();
    GroundTileRenderer.LoadVertices();

    GroundTileRenderer.SetProjection(width, height);
    GroundTileRenderer.SetModel();
    GroundTileRenderer.SetView(lookX, lookY);
    GroundTileRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    GroundTileRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    GroundTileRenderer.AddUniform("windowWidth", width);
    GroundTileRenderer.AddUniform("windowHeight", height);
    GroundTileRenderer.AddUniform("centerLat", Center.GetLat());
    GroundTileRenderer.AddUniform("centerLon", Center.GetLon());
    GroundTileRenderer.AddUniform("magnification", Magnification.GetMagnification());
    GroundTileRenderer.AddUniform("dpi", dpi);

    GroundRenderer.SwapData();

    GroundRenderer.BindBuffers();

    GroundRenderer.LoadProgram();
    GroundRenderer.LoadVertices();

    GroundRenderer.SetProjection(width, height);
    GroundRenderer.SetModel();
    GroundRenderer.SetView(lookX, lookY);
    GroundRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    GroundRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    GroundRenderer.AddUniform("centerLat", Center.GetLat());
    GroundRenderer.AddUniform("centerLon", Center.GetLon());
    GroundRenderer.AddUniform("magnification", Magnification.GetMagnification());
    GroundRenderer.AddUniform("dpi", dpi);

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
    PathRenderer.AddAttrib("barycentricX", 1, GL_FLOAT, 11 * sizeof(GLfloat));
    PathRenderer.AddAttrib("barycentricY", 1, GL_FLOAT, 12 * sizeof(GLfloat));
    PathRenderer.AddAttrib("barycentricZ", 1, GL_FLOAT, 13 * sizeof(GLfloat));
    PathRenderer.AddUniform("windowWidth", width);
    PathRenderer.AddUniform("windowHeight", height);
    PathRenderer.AddUniform("centerLat", Center.GetLat());
    PathRenderer.AddUniform("centerLon", Center.GetLon());
    PathRenderer.AddUniform("magnification", Magnification.GetMagnification());
    PathRenderer.AddUniform("dpi", dpi);

    ImageRenderer.SwapData();

    ImageRenderer.BindBuffers();
    ImageRenderer.LoadProgram();
    ImageRenderer.LoadVertices();
    ImageRenderer.LoadTextures();

    ImageRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    ImageRenderer.AddAttrib("index", 1, GL_FLOAT, 2 * sizeof(GLfloat));
    ImageRenderer.AddAttrib("textureStart", 1, GL_FLOAT, 3 * sizeof(GLfloat));
    ImageRenderer.AddAttrib("textureWidth", 1, GL_FLOAT, 4 * sizeof(GLfloat));
    ImageRenderer.AddUniform("windowWidth", width);
    ImageRenderer.AddUniform("windowHeight", height);
    ImageRenderer.AddUniform("centerLat", Center.GetLat());
    ImageRenderer.AddUniform("centerLon", Center.GetLon());
    ImageRenderer.AddUniform("quadWidth", 14);
    ImageRenderer.AddUniform("magnification", Magnification.GetMagnification());
    ImageRenderer.AddUniform("textureWidthSum", ImageRenderer.GetTextureWidth());
    ImageRenderer.AddUniform("dpi", dpi);

    ImageRenderer.SetProjection(width, height);
    ImageRenderer.SetModel();
    ImageRenderer.SetView(lookX, lookY);

    TextRenderer.SwapData(1);

    TextRenderer.BindBuffers();
    TextRenderer.LoadProgram();
    TextRenderer.LoadVertices();
    TextRenderer.SetTextureHeight(Textloader.GetHeight());
    TextRenderer.LoadGreyTextures();

    TextRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    TextRenderer.AddAttrib("color", 4, GL_FLOAT, 2 * sizeof(GLfloat));
    TextRenderer.AddAttrib("index", 1, GL_FLOAT, 6 * sizeof(GLfloat));
    TextRenderer.AddAttrib("textureStart", 1, GL_FLOAT, 7 * sizeof(GLfloat));
    TextRenderer.AddAttrib("textureWidth", 1, GL_FLOAT, 8 * sizeof(GLfloat));
    TextRenderer.AddAttrib("positionOffset", 1, GL_FLOAT, 9 * sizeof(GLfloat));
    TextRenderer.AddAttrib("fontSize", 1, GL_FLOAT, 10 * sizeof(GLfloat));
    TextRenderer.AddUniform("windowWidth", width);
    TextRenderer.AddUniform("windowHeight", height);
    TextRenderer.AddUniform("centerLat", Center.GetLat());
    TextRenderer.AddUniform("centerLon", Center.GetLon());
    TextRenderer.AddUniform("textureHeight", TextRenderer.GetTextureHeight());
    TextRenderer.AddUniform("magnification", Magnification.GetMagnification());
    TextRenderer.AddUniform("textureWidthSum", ImageRenderer.GetTextureWidth());
    TextRenderer.AddUniform("dpi", dpi);

    TextRenderer.SetProjection(width, height);
    TextRenderer.SetModel();
    TextRenderer.SetView(lookX, lookY);
  }

  void
  osmscout::MapPainterOpenGL::ProcessAreaData(const osmscout::MapData &data,
                                              const osmscout::MapParameter &/*parameter*/,
                                              const osmscout::Projection &projection,
                                              const osmscout::StyleConfigRef &styleConfig) {

    osmscout::log.Info() << "Area: " << data.areas.size();

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

          if (!osmscout::AreaIsSimple(p))
            continue;

          osmscout::GeoBox ringBoundingBox;
          ring.GetBoundingBox(ringBoundingBox);

          double borderWidth = borderStyle ? borderStyle->GetWidth() : 0.0;

          if (!IsVisibleArea(projection,
                             ringBoundingBox,
                             borderWidth / 2.0))
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

  bool osmscout::MapPainterOpenGL::IsVisibleArea(const Projection &projection, const GeoBox &boundingBox,
                                                 double pixelOffset) {
    double x1;
    double x2;
    double y1;
    double y2;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          x1,
                          y1);

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          x2,
                          y2);

    double xMin = std::min(x1, x2) - pixelOffset;
    double xMax = std::max(x1, x2) + pixelOffset;
    double yMin = std::min(y1, y2) - pixelOffset;
    double yMax = std::max(y1, y2) + pixelOffset;

    osmscout::GeoBox gb;
    projection.GetDimensions(gb);
    double areaMinDimension = projection.ConvertWidthToPixel(Parameter.GetAreaMinDimensionMM());

    if (xMax - xMin <= areaMinDimension &&
        yMax - yMin <= areaMinDimension) {
      return false;
    }

    return !(xMin >= projection.GetWidth() ||
             yMin >= projection.GetHeight() ||
             xMax < 0 ||
             yMax < 0);
  }

  void
  osmscout::MapPainterOpenGL::ProcessPathData(const osmscout::MapData &data,
                                              const osmscout::MapParameter &/*parameter*/,
                                              const osmscout::Projection &projection,
                                              const osmscout::StyleConfigRef &styleConfig) {

    WidthFeatureValueReader widthReader(*styleConfig->GetTypeConfig());
    LayerFeatureValueReader layerReader(*styleConfig->GetTypeConfig());

    osmscout::log.Info() << "Ways: " << data.ways.size();

    for (const auto &way: data.ways) {

      std::vector<LineStyleRef> styles;

      styleConfig->GetWayLineStyles(way->GetFeatureValueBuffer(),
                                    projection,
                                    styles);

      if (!styles.empty())
        lineStyles = styles;

      FeatureValueBuffer buffer(way->GetFeatureValueBuffer());

      for (const auto &lineStyle : lineStyles) {
        double lineWidth = 0.0;
        double lineOffset = 0.0;

        if (lineStyle->GetWidth() > 0.0) {
          WidthFeatureValue *widthValue = widthReader.GetValue(buffer);

          if (widthValue != NULL) {
            lineWidth += widthValue->GetWidth() / projection.GetPixelSize();
          } else {
            lineWidth += lineStyle->GetWidth() / projection.GetPixelSize();
          }
        }

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

        for (size_t i = 0; i < way->nodes.size() - 1; i++) {
          Color color = lineStyle->GetLineColor();
          //first triangle
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 1 : 5, lineWidth,
                        glm::vec3(1,0,1));
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 2 : 6, lineWidth,
                        glm::vec3(0,1,1));
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, (i == way->nodes.size() - 2 ? 7 : 3), lineWidth,
                        glm::vec3(0,0,1));
          //second triangle
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, (i == way->nodes.size() - 2) ? 7 : 3, lineWidth,
                        glm::vec3(1,1,0));
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 2 : 6, lineWidth,
                        glm::vec3(0,1,0));
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 8 : 4, lineWidth,
                        glm::vec3(0,1,1));

          int num;
          num = PathRenderer.GetVerticesNumber() - 6;
          PathRenderer.AddNewElement(num);
          PathRenderer.AddNewElement(num + 1);
          PathRenderer.AddNewElement(num + 2);
          PathRenderer.AddNewElement(num + 3);
          PathRenderer.AddNewElement(num + 4);
          PathRenderer.AddNewElement(num + 5);


          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 1 : 5, lineWidth,
                        glm::vec3(1,1,0));
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 8 : 4, lineWidth,
                        glm::vec3(0,1,0));
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 2 : 6, lineWidth,
                        glm::vec3(0,1,1));
          //
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 1 : 5, lineWidth,
                        glm::vec3(1,0,0));
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 8 : 4, lineWidth,
                        glm::vec3(1,1,0));
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 7 : 3, lineWidth,
                        glm::vec3(1,0,1));

          num = PathRenderer.GetVerticesNumber() - 6;
          PathRenderer.AddNewElement(num);
          PathRenderer.AddNewElement(num + 1);
          PathRenderer.AddNewElement(num + 2);
          PathRenderer.AddNewElement(num + 3);
          PathRenderer.AddNewElement(num + 4);
          PathRenderer.AddNewElement(num + 5);

        }
      }
    }
  }

  void
  osmscout::MapPainterOpenGL::AddPathVertex(osmscout::Point current, osmscout::Point previous, osmscout::Point next,
                                            osmscout::Color color, int type, float width, glm::vec3 barycentric) {
    PathRenderer.AddNewVertex(current.GetLon());
    PathRenderer.AddNewVertex(current.GetLat());

    PathRenderer.AddNewVertex(previous.GetLon());
    PathRenderer.AddNewVertex(previous.GetLat());

    PathRenderer.AddNewVertex(next.GetLon());
    PathRenderer.AddNewVertex(next.GetLat());

    PathRenderer.AddNewVertex(color.GetR());
    PathRenderer.AddNewVertex(color.GetG());
    PathRenderer.AddNewVertex(color.GetB());

    PathRenderer.AddNewVertex(type);

    PathRenderer.AddNewVertex(width);

    PathRenderer.AddNewVertex(barycentric.x);
    PathRenderer.AddNewVertex(barycentric.y);
    PathRenderer.AddNewVertex(barycentric.z);
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

  void
  osmscout::MapPainterOpenGL::ProcessNodeData(const osmscout::MapData &data,
                                              const osmscout::MapParameter &parameter,
                                              const osmscout::Projection &projection,
                                              const osmscout::StyleConfigRef &styleConfig) {
    LabelLayouter labels;
    labels.Initialize(projection, parameter);

    osmscout::log.Info() << "Nodes: " << data.nodes.size();

    std::vector<int> icons;
    for (const auto &node: data.nodes) {
      FeatureValueBuffer buffer = node->GetFeatureValueBuffer();
      IconStyleRef iconStyle;
      styleConfig->GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                    projection,
                                    iconStyle);

      std::vector<TextStyleRef> textStyles;
      styleConfig->GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                     projection,
                                     textStyles);

      if (iconStyle) {
        //has icon?
        bool hasIcon = false;
        OpenGLTexture *image;
        int IconIndex = 0;
        for (std::list<std::string>::const_iterator path = parameter.GetIconPaths().begin();
             path != parameter.GetIconPaths().end();
             ++path) {
          std::string filename = *path + iconStyle->GetIconName() + ".png";

          int id = iconStyle->GetIconId();
          bool loaded = false;
          for (unsigned int i = 0; i < icons.size(); i++) {
            if (id == icons[i]) {
              IconIndex = i;
              hasIcon = true;
              loaded = true;
              break;
            }
          }

          if (loaded)
            break;

          image = osmscout::LoadPNGOpenGL(filename);

          if (image != NULL) {
            ImageRenderer.AddNewTexture(image);
            icons.push_back(id);
            hasIcon = true;
            IconIndex = icons.size() - 1;
            break;
          }
        }

        if (!iconStyle->GetIconName().empty() && hasIcon) {
          osmscout::GeoCoord coords = node->GetCoords();
          size_t textureWidth = ImageRenderer.GetTextureWidth(IconIndex);
          size_t startWidth = ImageRenderer.GetTextureWidthSum(IconIndex) - textureWidth;

          ImageRenderer.AddNewVertex(coords.GetLon());
          ImageRenderer.AddNewVertex(coords.GetLat());
          ImageRenderer.AddNewVertex(1);
          ImageRenderer.AddNewVertex(startWidth);
          ImageRenderer.AddNewVertex(textureWidth);

          ImageRenderer.AddNewVertex(coords.GetLon());
          ImageRenderer.AddNewVertex(coords.GetLat());
          ImageRenderer.AddNewVertex(2);
          ImageRenderer.AddNewVertex(startWidth);
          ImageRenderer.AddNewVertex(textureWidth);

          ImageRenderer.AddNewVertex(coords.GetLon());
          ImageRenderer.AddNewVertex(coords.GetLat());
          ImageRenderer.AddNewVertex(3);
          ImageRenderer.AddNewVertex(startWidth);
          ImageRenderer.AddNewVertex(textureWidth);

          ImageRenderer.AddNewVertex(coords.GetLon());
          ImageRenderer.AddNewVertex(coords.GetLat());
          ImageRenderer.AddNewVertex(3);
          ImageRenderer.AddNewVertex(startWidth);
          ImageRenderer.AddNewVertex(textureWidth);

          ImageRenderer.AddNewVertex(coords.GetLon());
          ImageRenderer.AddNewVertex(coords.GetLat());
          ImageRenderer.AddNewVertex(1);
          ImageRenderer.AddNewVertex(startWidth);
          ImageRenderer.AddNewVertex(textureWidth);

          ImageRenderer.AddNewVertex(coords.GetLon());
          ImageRenderer.AddNewVertex(coords.GetLat());
          ImageRenderer.AddNewVertex(4);
          ImageRenderer.AddNewVertex(startWidth);
          ImageRenderer.AddNewVertex(textureWidth);

          int num;
          if (ImageRenderer.GetNumOfVertices() <= 30) {
            num = 0;
          } else {
            num = ImageRenderer.GetVerticesNumber() - 6;
          }
          ImageRenderer.AddNewElement(num);
          ImageRenderer.AddNewElement(num + 1);
          ImageRenderer.AddNewElement(num + 2);
          ImageRenderer.AddNewElement(num + 3);
          ImageRenderer.AddNewElement(num + 4);
          ImageRenderer.AddNewElement(num + 5);


        } else if (iconStyle->GetSymbol()) {
          osmscout::SymbolRef symbol = iconStyle->GetSymbol();

          double minX;
          double minY;
          double maxX;
          double maxY;
          symbol->GetBoundingBox(minX, minY, maxX, maxY);

          double centerX = (minX + maxX) / 2;
          double centerY = (minY + maxY) / 2;

          for (const auto &p : symbol->GetPrimitives()) {
            DrawPrimitive *primitive = p.get();
            FillStyleRef fillStyle = primitive->GetFillStyle();

            if (dynamic_cast<PolygonPrimitive *>(primitive) != NULL) {
              PolygonPrimitive *polygon = dynamic_cast<PolygonPrimitive *>(primitive);
              double meterPerPixelLat = (40075.016686 * 1000) * std::cos(node->GetCoords().GetLat()) /
                                        (float) (std::pow(2, (Magnification.GetLevel() + 9)));
              double meterPerPixel = (40075.016686 * 1000) / (float) (std::pow(2, (Magnification.GetLevel() + 9)));
              std::vector<osmscout::Vertex2D> vertices;
              for (const auto &pixel : polygon->GetCoords()) {
                double meterToDegreeLat = std::cos(node->GetCoords().GetLat()) * 0.00001;
                double meterToDegree = 0.00001;
                double scale = -1 * meterPerPixel * meterToDegree;
                double scaleLat = -1 * meterPerPixelLat * meterToDegreeLat;

                double x =
                    node->GetCoords().GetLon() + (projection.ConvertWidthToPixel(pixel.GetX() - centerX) * scale);
                double y = node->GetCoords().GetLat() +
                           (projection.ConvertWidthToPixel(maxY - pixel.GetY() - centerY) * scaleLat);

                vertices.push_back(osmscout::Vertex2D(x, y));
              }

              std::vector<GLfloat> points = osmscout::Triangulate::TriangulatePolygon(vertices);

              Color color = fillStyle->GetFillColor();

              for (size_t t = 0; t < points.size(); t++) {
                if (t % 2 == 0) {
                  AreaRenderer.AddNewVertex(points[t]);
                } else {
                  AreaRenderer.AddNewVertex(points[t]);
                  AreaRenderer.AddNewVertex(color.GetR());
                  AreaRenderer.AddNewVertex(color.GetG());
                  AreaRenderer.AddNewVertex(color.GetB());

                  if (AreaRenderer.GetNumOfVertices() <= 5) {
                    AreaRenderer.AddNewElement(0);
                  } else {
                    AreaRenderer.AddNewElement(AreaRenderer.GetVerticesNumber() - 1);
                  }
                }
              }
            }
          }
        }
      }

      for (const auto textStyle : textStyles) {
        std::string label = textStyle->GetLabel()->GetLabel(parameter,
                                                            buffer);

        if (label.empty()) {
          continue;
        }

        double alpha = 1.0;
        double fontSize = 1.0;

        if (projection.GetMagnification() > textStyle->GetScaleAndFadeMag() &&
            parameter.GetDrawFadings()) {
          double factor = projection.GetMagnification().GetLevel() - textStyle->GetScaleAndFadeMag().GetLevel();
          fontSize = textStyle->GetSize() * pow(1.5, factor);
          alpha = std::min(textStyle->GetAlpha() / factor, 1.0);

        } else if (textStyle->GetAutoSize()) {
          alpha = textStyle->GetAlpha();
          //TODO
          continue;
        } else {
          fontSize = textStyle->GetSize();
          alpha = textStyle->GetAlpha();
        }

        Color color = textStyle->GetTextColor();
        std::vector<int> textureAtlasIndices = Textloader.AddCharactersToTextureAtlas(label);
        int widthSum = 0;
        for (int index: textureAtlasIndices) {
          osmscout::GeoCoord coords = node->GetCoords();
          size_t textureWidth = Textloader.GetWidth(index);
          size_t startWidth = Textloader.GetStartWidth(index);

          int shaderIndices[] = {1, 2, 3, 3, 1, 4};
          for (int i: shaderIndices) {
            TextRenderer.AddNewVertex(coords.GetLon());
            TextRenderer.AddNewVertex(coords.GetLat());
            TextRenderer.AddNewVertex(color.GetR());
            TextRenderer.AddNewVertex(color.GetG());
            TextRenderer.AddNewVertex(color.GetB());
            TextRenderer.AddNewVertex(alpha);
            TextRenderer.AddNewVertex(i);
            TextRenderer.AddNewVertex(startWidth);
            TextRenderer.AddNewVertex(textureWidth);
            TextRenderer.AddNewVertex(widthSum);
            TextRenderer.AddNewVertex(fontSize);
          }

          widthSum += textureWidth + 1;

          int num;
          if (TextRenderer.GetNumOfVertices() <= 60) {
            num = 0;
          } else {
            num = TextRenderer.GetVerticesNumber() - 6;
          }
          TextRenderer.AddNewElement(num);
          TextRenderer.AddNewElement(num + 1);
          TextRenderer.AddNewElement(num + 2);
          TextRenderer.AddNewElement(num + 3);
          TextRenderer.AddNewElement(num + 4);
          TextRenderer.AddNewElement(num + 5);

        }
      }
    }

    OpenGLTexture *t = Textloader.CreateTexture();
    TextRenderer.AddNewTexture(t);

  }

  void osmscout::MapPainterOpenGL::OnZoom(float zoomDirection) {
    if (zoomDirection < 0)
      Magnification.SetLevel(Magnification.GetLevel() - 1);
    else if (zoomDirection > 0)
      Magnification.SetLevel(Magnification.GetLevel() + 1);
  }

  void osmscout::MapPainterOpenGL::OnTranslation(int startPointX, int startPointY, int endPointX, int endPointY) {
    double endLat, endLon, startLat, startLon;
    PixelToGeo(startPointX, startPointY, startLon, startLat);
    PixelToGeo(endPointX, endPointY, endLon, endLat);
    double offsetX = (startLon - endLon);
    double offsetY = (startLat - endLat);

    GeoCoord g = osmscout::GeoCoord(Center.GetLat() + offsetY / 2, Center.GetLon() + offsetX / 2);
    Center = g;
  }

  bool osmscout::MapPainterOpenGL::PixelToGeo(double x, double y,
                                              double &lon, double &lat) {
    double tileDPI = 96.0;
    double gradtorad = 2 * M_PI / 360;
    double earthRadiusMeter = 6378137.0;
    double earthExtentMeter = 2 * M_PI * earthRadiusMeter;
    double tileWidthZoom0Aquator = earthExtentMeter;
    double equatorTileWidth = tileWidthZoom0Aquator / Magnification.GetMagnification();
    double equatorTileResolution = equatorTileWidth / 256.0;
    double equatorCorrectedEquatorTileResolution = equatorTileResolution * tileDPI / dpi;
    double groundWidthEquatorMeter = width * equatorCorrectedEquatorTileResolution;
    double latOffset = atanh(sin(Center.GetLat() * gradtorad));

    double scale = width / (2 * M_PI * groundWidthEquatorMeter / earthExtentMeter);
    double scaleGradtorad = scale * gradtorad;

    x -= width / 2;
    y = height / 2 - y;

    lon = Center.GetLon() + x / scaleGradtorad;
    lat = atan(sinh(y / scale + latOffset)) / gradtorad;

    return true;
  }

  osmscout::GeoCoord osmscout::MapPainterOpenGL::GetCenter() {
    return Center;
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

    GroundTileRenderer.AddUniform("windowWidth", width);
    GroundTileRenderer.AddUniform("windowHeight", height);
    GroundTileRenderer.AddUniform("centerLat", Center.GetLat());
    GroundTileRenderer.AddUniform("centerLon", Center.GetLon());
    GroundTileRenderer.AddUniform("magnification", Magnification.GetMagnification());
    GroundTileRenderer.AddUniform("dpi", dpi);

    GroundTileRenderer.SetProjection(width, height);
    GroundTileRenderer.SetModel();
    GroundTileRenderer.SetView(lookX, lookY);
    GroundTileRenderer.Draw();

    glBindVertexArray(GroundRenderer.getVAO());
    glUseProgram(GroundRenderer.getShaderProgram());

    GroundRenderer.AddUniform("windowWidth", width);
    GroundRenderer.AddUniform("windowHeight", height);
    GroundRenderer.AddUniform("centerLat", Center.GetLat());
    GroundRenderer.AddUniform("centerLon", Center.GetLon());
    GroundRenderer.AddUniform("magnification", Magnification.GetMagnification());
    GroundRenderer.AddUniform("dpi", dpi);

    GroundRenderer.SetProjection(width, height);
    GroundRenderer.SetModel();
    GroundRenderer.SetView(lookX, lookY);
    GroundRenderer.Draw();

    glBindVertexArray(AreaRenderer.getVAO());
    glUseProgram(AreaRenderer.getShaderProgram());

    AreaRenderer.AddUniform("windowWidth", width);
    AreaRenderer.AddUniform("windowHeight", height);
    AreaRenderer.AddUniform("centerLat", Center.GetLat());
    AreaRenderer.AddUniform("centerLon", Center.GetLon());
    AreaRenderer.AddUniform("magnification", Magnification.GetMagnification());
    AreaRenderer.AddUniform("dpi", dpi);

    AreaRenderer.SetProjection(width, height);
    AreaRenderer.SetModel();
    AreaRenderer.SetView(lookX, lookY);
    AreaRenderer.Draw();

    glBindVertexArray(PathRenderer.getVAO());
    glUseProgram(PathRenderer.getShaderProgram());

    PathRenderer.AddUniform("windowWidth", width);
    PathRenderer.AddUniform("windowHeight", height);
    PathRenderer.AddUniform("centerLat", Center.GetLat());
    PathRenderer.AddUniform("centerLon", Center.GetLon());
    PathRenderer.AddUniform("magnification", Magnification.GetMagnification());
    PathRenderer.AddUniform("dpi", dpi);

    PathRenderer.SetProjection(width, height);
    PathRenderer.SetModel();
    PathRenderer.SetView(lookX, lookY);
    PathRenderer.Draw();

    glBindVertexArray(ImageRenderer.getVAO());
    glBindTexture(GL_TEXTURE_2D, ImageRenderer.GetTexture());
    glUseProgram(ImageRenderer.getShaderProgram());

    ImageRenderer.AddUniform("windowWidth", width);
    ImageRenderer.AddUniform("windowHeight", height);
    ImageRenderer.AddUniform("centerLat", Center.GetLat());
    ImageRenderer.AddUniform("centerLon", Center.GetLon());
    ImageRenderer.AddUniform("quadWidth", 14);
    ImageRenderer.AddUniform("magnification", Magnification.GetMagnification());
    ImageRenderer.AddUniform("textureWidthSum", ImageRenderer.GetTextureWidth());
    ImageRenderer.AddUniform("dpi", dpi);

    ImageRenderer.SetProjection(width, height);
    ImageRenderer.SetModel();
    ImageRenderer.SetView(lookX, lookY);
    ImageRenderer.Draw();

    glBindVertexArray(TextRenderer.getVAO());
    glBindTexture(GL_TEXTURE_2D, TextRenderer.GetTexture());
    glUseProgram(TextRenderer.getShaderProgram());

    TextRenderer.AddUniform("windowWidth", width);
    TextRenderer.AddUniform("windowHeight", height);
    TextRenderer.AddUniform("centerLat", Center.GetLat());
    TextRenderer.AddUniform("centerLon", Center.GetLon());
    TextRenderer.AddUniform("textureHeight", TextRenderer.GetTextureHeight());
    TextRenderer.AddUniform("magnification", Magnification.GetMagnification());
    TextRenderer.AddUniform("textureWidthSum", TextRenderer.GetTextureWidth());
    TextRenderer.AddUniform("dpi", dpi);

    TextRenderer.SetProjection(width, height);
    TextRenderer.SetModel();
    TextRenderer.SetView(lookX, lookY);
    TextRenderer.Draw();

  }

}
