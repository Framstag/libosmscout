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
        Textloader(fontPath, 10) {
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

    WayRenderer.LoadVertexShader("PathVertexShader.vert");
    WayRenderer.LoadFragmentShader("PathFragmentShader.frag");
    success = WayRenderer.InitContext();
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
    AreaRenderer.SetVerticesSize(6);
    GroundTileRenderer.clearData();
    GroundTileRenderer.SetVerticesSize(5);
    GroundRenderer.clearData();
    GroundRenderer.SetVerticesSize(5);
    WayRenderer.clearData();
    WayRenderer.SetVerticesSize(23);
    ImageRenderer.clearData();
    ImageRenderer.SetVerticesSize(5);
    ImageRenderer.SetTextureHeight(7);
    TextRenderer.clearData();
    TextRenderer.SetVerticesSize(11);
  }

  void osmscout::MapPainterOpenGL::ProcessData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                            const osmscout::Projection &projection,
                                            const osmscout::StyleConfigRef &styleConfig) {
    styleConfig.get()->GetLandFillStyle(projection, landFill);
    styleConfig.get()->GetSeaFillStyle(projection, seaFill);

    Textloader.SetDefaultFontSize(parameter.GetFontSize());

    this->Magnification = projection.GetMagnification();
    this->Center = projection.GetCenter();
    this->Parameter = parameter;

    ProcessAreas(data, parameter, projection, styleConfig);

    ProcessGround(data, parameter, projection, styleConfig);

    ProcessWays(data, parameter, projection, styleConfig);

    ProcessNodes(data, parameter, projection, styleConfig);
  }

  void osmscout::MapPainterOpenGL::SwapData() {

    SwapAreaData();
    SwapGroundData();
    SwapWayData();
    SwapNodeData();
  }

  void osmscout::MapPainterOpenGL::SwapAreaData() {
    AreaRenderer.SwapData();

    AreaRenderer.BindBuffers();
    AreaRenderer.LoadProgram();
    AreaRenderer.LoadVertices();

    AreaRenderer.SetProjection(width, height);
    AreaRenderer.SetModel();
    AreaRenderer.SetView(lookX, lookY);
    AreaRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    AreaRenderer.AddAttrib("color", 4, GL_FLOAT, 2 * sizeof(GLfloat));

    AreaRenderer.AddUniform("windowWidth", width);
    AreaRenderer.AddUniform("windowHeight", height);
    AreaRenderer.AddUniform("centerLat", Center.GetLat());
    AreaRenderer.AddUniform("centerLon", Center.GetLon());
    AreaRenderer.AddUniform("magnification", Magnification.GetMagnification());
    AreaRenderer.AddUniform("dpi", dpi);
  }

  void osmscout::MapPainterOpenGL::SwapGroundData() {
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
  }

  void osmscout::MapPainterOpenGL::SwapNodeData() {
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
    ImageRenderer.AddUniform("z", 0.001);

    ImageRenderer.SetProjection(width, height);
    ImageRenderer.SetModel();
    ImageRenderer.SetView(lookX, lookY);

    TextRenderer.SetTextureHeight(Textloader.GetHeight());
    TextRenderer.SwapData(1);

    TextRenderer.BindBuffers();
    TextRenderer.LoadProgram();
    TextRenderer.LoadVertices();
    TextRenderer.LoadGreyTextures();

    TextRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    TextRenderer.AddAttrib("color", 4, GL_FLOAT, 2 * sizeof(GLfloat));
    TextRenderer.AddAttrib("index", 1, GL_FLOAT, 6 * sizeof(GLfloat));
    TextRenderer.AddAttrib("textureStart", 1, GL_FLOAT, 7 * sizeof(GLfloat));
    TextRenderer.AddAttrib("textureWidth", 1, GL_FLOAT, 8 * sizeof(GLfloat));
    TextRenderer.AddAttrib("positionOffset", 1, GL_FLOAT, 9 * sizeof(GLfloat));
    TextRenderer.AddAttrib("startOffset", 1, GL_FLOAT, 10 * sizeof(GLfloat));
    TextRenderer.AddUniform("windowWidth", width);
    TextRenderer.AddUniform("windowHeight", height);
    TextRenderer.AddUniform("centerLat", Center.GetLat());
    TextRenderer.AddUniform("centerLon", Center.GetLon());
    TextRenderer.AddUniform("textureHeight", Textloader.GetHeight());
    TextRenderer.AddUniform("magnification", Magnification.GetMagnification());
    TextRenderer.AddUniform("textureWidthSum", ImageRenderer.GetTextureWidth());
    TextRenderer.AddUniform("dpi", dpi);
    TextRenderer.AddUniform("z", 0.001);

    TextRenderer.SetProjection(width, height);
    TextRenderer.SetModel();
    TextRenderer.SetView(lookX, lookY);
  }

  void osmscout::MapPainterOpenGL::SwapWayData() {
    WayRenderer.SwapData();

    WayRenderer.BindBuffers();
    WayRenderer.LoadProgram();
    WayRenderer.LoadVertices();

    WayRenderer.SetProjection(width, height);
    WayRenderer.SetModel();
    WayRenderer.SetView(lookX, lookY);
    WayRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    WayRenderer.AddAttrib("previous", 2, GL_FLOAT, 2 * sizeof(GLfloat));
    WayRenderer.AddAttrib("next", 2, GL_FLOAT, 4 * sizeof(GLfloat));
    WayRenderer.AddAttrib("color", 4, GL_FLOAT, 6 * sizeof(GLfloat));
    WayRenderer.AddAttrib("gapcolor", 4, GL_FLOAT, 10 * sizeof(GLfloat));
    WayRenderer.AddAttrib("index", 1, GL_FLOAT, 14 * sizeof(GLfloat));
    WayRenderer.AddAttrib("thickness", 1, GL_FLOAT, 15 * sizeof(GLfloat));
    WayRenderer.AddAttrib("border", 1, GL_FLOAT, 16 * sizeof(GLfloat));
    WayRenderer.AddAttrib("barycentric", 3, GL_FLOAT, 17 * sizeof(GLfloat));
    WayRenderer.AddAttrib("z", 1, GL_FLOAT, 20 * sizeof(GLfloat));
    WayRenderer.AddAttrib("dashsize", 1, GL_FLOAT, 21 * sizeof(GLfloat));
    WayRenderer.AddAttrib("length", 1, GL_FLOAT, 22 * sizeof(GLfloat));
    WayRenderer.AddUniform("windowWidth", width);
    WayRenderer.AddUniform("windowHeight", height);
    WayRenderer.AddUniform("centerLat", Center.GetLat());
    WayRenderer.AddUniform("centerLon", Center.GetLon());
    WayRenderer.AddUniform("magnification", Magnification.GetMagnification());
    WayRenderer.AddUniform("dpi", dpi);
  }

  void
  osmscout::MapPainterOpenGL::ProcessAreas(const osmscout::MapData &data,
                                           const osmscout::MapParameter &/*parameter*/,
                                           const osmscout::Projection &projection,
                                           const osmscout::StyleConfigRef &styleConfig) {

    osmscout::log.Info() << "Area: " << data.areas.size();

    std::vector<AreaRef> areas = data.areas;

    std::sort(areas.begin(), areas.end(),
              [](const AreaRef &a, const AreaRef &b) -> bool {
                GeoBox b1;
                GeoBox b2;
                a->GetBoundingBox(b1);
                b->GetBoundingBox(b2);
                return b1.GetHeight() * b1.GetWidth() > b2.GetHeight() * b2.GetWidth();
              });

    for (const auto &area : areas) {
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

          size_t j = i + 1;
          int hasClippings = 0;
          while (j < area->rings.size() &&
                 area->rings[j].GetRing() == ringId + 1 &&
                 area->rings[j].GetType()->GetIgnore()) {
            r.push_back(area->rings[j]);
            j++;
            hasClippings = 1;
          }

          std::vector<GLfloat> points;

          if (!fillStyle && borderStyles.empty()) {
            continue;
          }

          Color c = fillStyle->GetFillColor();

          BorderStyleRef borderStyle;
          size_t borderStyleIndex = 0;

          if (!borderStyles.empty() &&
              borderStyles.front()->GetDisplayOffset() == 0.0 &&
              borderStyles.front()->GetOffset() == 0.0) {
            borderStyle = borderStyles[borderStyleIndex];
            borderStyleIndex++;
          }

          double borderWidth = borderStyle ? borderStyle->GetWidth() : 0.0;

          if (!IsVisibleArea(projection,
                             ringBoundingBox,
                             borderWidth / 2.0))
            continue;


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
              if (ring.nodes.size() >= 3) {
                polygons.push_back(ring.nodes);
              }
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
              AreaRenderer.AddNewVertex(c.GetA());

              if (AreaRenderer.GetNumOfVertices() <= 6) {
                AreaRenderer.AddNewElement(0);
              } else {
                AreaRenderer.AddNewElement(AreaRenderer.GetVerticesNumber() - 1);
              }
            }
          }

          p.push_back(p[0]);
          for (size_t idx = 0;
               idx < borderStyles.size();
               idx++) {
            borderStyle = borderStyles[idx];

            for (size_t t = 0; t < p.size() - 1; t++) {

              Color color = borderStyle->GetColor();
              //first triangle
              AddPathVertex(p[t],
                            t == 0 ? p[t] : p[t - 1],
                            p[t + 1],
                            color, t == 0 ? 1 : 5, borderWidth,
                            glm::vec3(1, 0, 0));
              AddPathVertex(p[t],
                            t == 0 ? p[t] : p[t - 1],
                            p[t + 1],
                            color, t == 0 ? 2 : 6, borderWidth,
                            glm::vec3(0, 1, 0));
              AddPathVertex(p[t + 1],
                            p[t],
                            p[t + 2],
                            color, (t == p.size() - 2 ? 7 : 3), borderWidth,
                            glm::vec3(0, 0, 1));
              //second triangle
              AddPathVertex(p[t + 1],
                            p[t],
                            p[t + 2],
                            color, (t == p.size() - 2) ? 7 : 3, borderWidth,
                            glm::vec3(1, 0, 0));
              AddPathVertex(p[t],
                            t == 0 ? p[t] : p[t - 1],
                            p[t + 1],
                            color, t == 0 ? 2 : 6, borderWidth,
                            glm::vec3(0, 1, 0));
              AddPathVertex(p[t + 1],
                            p[t],
                            p[t + 2],
                            color, t == p.size() - 2 ? 8 : 4, borderWidth,
                            glm::vec3(0, 0, 1));

              int num;
              num = WayRenderer.GetVerticesNumber() - 6;
              WayRenderer.AddNewElement(num);
              WayRenderer.AddNewElement(num + 1);
              WayRenderer.AddNewElement(num + 2);
              WayRenderer.AddNewElement(num + 3);
              WayRenderer.AddNewElement(num + 4);
              WayRenderer.AddNewElement(num + 5);
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
  osmscout::MapPainterOpenGL::ProcessWays(const osmscout::MapData &data,
                                          const osmscout::MapParameter &/*parameter*/,
                                          const osmscout::Projection &projection,
                                          const osmscout::StyleConfigRef &styleConfig) {

    WidthFeatureValueReader widthReader(*styleConfig->GetTypeConfig());
    LayerFeatureValueReader layerReader(*styleConfig->GetTypeConfig());

    osmscout::log.Info() << "Ways: " << data.ways.size();

    for (const auto &way: data.ways) {

      std::vector<LineStyleRef> lineStyles;

      styleConfig->GetWayLineStyles(way->GetFeatureValueBuffer(),
                                    projection,
                                    lineStyles);

      if (lineStyles.empty()) {
        continue;
      }

      FeatureValueBuffer buffer(way->GetFeatureValueBuffer());

      for (int l = lineStyles.size() - 1; l >= 0; l--) {
        Color color = lineStyles[l]->GetLineColor();

        int border = l;
        double lineWidth = 0.0;
        double lineOffset = 0.0;
        double z;
        if (l == 0)
          z = 0.001;
        else
          z = 0.0;

        if (lineStyles[l]->GetWidth() > 0.0) {
          WidthFeatureValue *widthValue = widthReader.GetValue(buffer);

          if (widthValue != NULL) {
            lineWidth += widthValue->GetWidth() / projection.GetPixelSize();
          } else {
            lineWidth += lineStyles[l]->GetWidth() / projection.GetPixelSize();
          }
        }

        if (lineStyles[l]->GetDisplayWidth() > 0.0) {
          lineWidth += projection.ConvertWidthToPixel(lineStyles[l]->GetDisplayWidth());
        }

        if (lineWidth == 0.0) {
          return;
        }

        if (lineStyles[l]->GetOffset() != 0.0) {
          lineOffset += lineStyles[l]->GetOffset() / projection.GetPixelSize();
        }

        if (lineStyles[l]->GetDisplayOffset() != 0.0) {
          lineOffset += projection.ConvertWidthToPixel(lineStyles[l]->GetDisplayOffset());
        }

        osmscout::Color gapColor;
        if (!lineStyles[l]->GetDash().empty())
          gapColor = lineStyles[l]->GetGapColor();
        else
          gapColor = lineStyles[l]->GetLineColor();

        for (size_t i = 0; i < way->nodes.size() - 1; i++) {
          double length = 1;
          double dashSize = 0;
          if (!lineStyles[l]->GetDash().empty() && (l == 0)) {
            for (int d = 0; d < lineStyles[l]->GetDash().size(); d++) {
              if (lineStyles[l]->GetDash()[d] != 0) {
                dashSize = lineStyles[l]->GetDash()[d];
                break;
              }
            }
            double distance = sqrt(osmscout::DistanceSquare(way->nodes[i], way->nodes[i + 1]));
            double degreeToMeter = std::abs(0.00001 * std::cos(way->nodes[i].GetLat()));
            double distanceMeter = distance / degreeToMeter;
            double result = projection.GetMeterInPixel() * distanceMeter;
            length = result;
          }
          //first triangle
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 1 : 5, lineWidth,
                        glm::vec3(1, 0, 1),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 2 : 6, lineWidth,
                        glm::vec3(0, 1, 1),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, (i == way->nodes.size() - 2 ? 7 : 3), lineWidth,
                        glm::vec3(0, 0, 1),
                        border, z, dashSize, length, gapColor);
          //second triangle
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, (i == way->nodes.size() - 2) ? 7 : 3, lineWidth,
                        glm::vec3(1, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 2 : 6, lineWidth,
                        glm::vec3(0, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 8 : 4, lineWidth,
                        glm::vec3(0, 1, 1),
                        border, z, dashSize, length, gapColor);

          int num;
          num = WayRenderer.GetVerticesNumber() - 6;
          for (unsigned int n = 0; n < 6; n++)
            WayRenderer.AddNewElement(num + n);

          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 1 : 5, lineWidth,
                        glm::vec3(1, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 8 : 4, lineWidth,
                        glm::vec3(0, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 2 : 6, lineWidth,
                        glm::vec3(0, 1, 1),
                        border, z, dashSize, length, gapColor);
          //
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? 1 : 5, lineWidth,
                        glm::vec3(1, 0, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 8 : 4, lineWidth,
                        glm::vec3(1, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        way->nodes[i + 2],
                        color, i == way->nodes.size() - 2 ? 7 : 3, lineWidth,
                        glm::vec3(1, 0, 1),
                        border, z, dashSize, length, gapColor);

          num = WayRenderer.GetVerticesNumber() - 6;
          for (unsigned int n = 0; n < 6; n++)
            WayRenderer.AddNewElement(num + n);
        }
      }
    }
  }

  void
  osmscout::MapPainterOpenGL::AddPathVertex(osmscout::Point current, osmscout::Point previous, osmscout::Point next,
                                            osmscout::Color color, int type, float width, glm::vec3 barycentric,
                                            int border, double z, float dashsize, float length,
                                            osmscout::Color gapcolor) {
    WayRenderer.AddNewVertex(current.GetLon());
    WayRenderer.AddNewVertex(current.GetLat());

    WayRenderer.AddNewVertex(previous.GetLon());
    WayRenderer.AddNewVertex(previous.GetLat());

    WayRenderer.AddNewVertex(next.GetLon());
    WayRenderer.AddNewVertex(next.GetLat());

    WayRenderer.AddNewVertex(color.GetR());
    WayRenderer.AddNewVertex(color.GetG());
    WayRenderer.AddNewVertex(color.GetB());
    WayRenderer.AddNewVertex(color.GetA());

    WayRenderer.AddNewVertex(gapcolor.GetR());
    WayRenderer.AddNewVertex(gapcolor.GetG());
    WayRenderer.AddNewVertex(gapcolor.GetB());
    WayRenderer.AddNewVertex(gapcolor.GetA());

    WayRenderer.AddNewVertex(type);

    WayRenderer.AddNewVertex(width);

    WayRenderer.AddNewVertex(border);

    WayRenderer.AddNewVertex(barycentric.x);
    WayRenderer.AddNewVertex(barycentric.y);
    WayRenderer.AddNewVertex(barycentric.z);

    WayRenderer.AddNewVertex(z);

    WayRenderer.AddNewVertex(dashsize);

    WayRenderer.AddNewVertex(length);
  }

  void
  osmscout::MapPainterOpenGL::ProcessGround(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
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
  osmscout::MapPainterOpenGL::ProcessNodes(const osmscout::MapData &data,
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

      bool hasIcon = false;
      if (iconStyle) {
        //has icon?
        OpenGLTextureRef image;
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
                  AreaRenderer.AddNewVertex(color.GetA());

                  if (AreaRenderer.GetNumOfVertices() <= 6) {
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

        int offset = 0;

        if (label.empty()) {
          continue;
        }

        if (hasIcon)
          offset = 15;

        double alpha = 1.0;
        double fontSize = 1.0;

        if (projection.GetMagnification() > textStyle->GetScaleAndFadeMag() &&
            parameter.GetDrawFadings()) {
          double factor = projection.GetMagnification().GetLevel() - textStyle->GetScaleAndFadeMag().GetLevel();
          fontSize = textStyle->GetSize() * pow(1.5, factor);
          alpha = std::min(textStyle->GetAlpha() / factor, 1.0);

        } else if (textStyle->GetAutoSize()) {
          //fontSize = textStyle->GetSize();
          alpha = textStyle->GetAlpha();
          //TODO
          continue;
        } else {
          fontSize = textStyle->GetSize();
          alpha = textStyle->GetAlpha();
        }

        Color color = textStyle->GetTextColor();
        std::vector<int> textureAtlasIndices = Textloader.AddCharactersToTextureAtlas(label, fontSize);
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
            TextRenderer.AddNewVertex(offset);
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

    OpenGLTextureRef t = Textloader.CreateTexture();
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

    glBindVertexArray(WayRenderer.getVAO());
    glUseProgram(WayRenderer.getShaderProgram());

    WayRenderer.AddUniform("windowWidth", width);
    WayRenderer.AddUniform("windowHeight", height);
    WayRenderer.AddUniform("centerLat", Center.GetLat());
    WayRenderer.AddUniform("centerLon", Center.GetLon());
    WayRenderer.AddUniform("magnification", Magnification.GetMagnification());
    WayRenderer.AddUniform("dpi", dpi);

    WayRenderer.SetProjection(width, height);
    WayRenderer.SetModel();
    WayRenderer.SetView(lookX, lookY);
    WayRenderer.Draw();

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
    ImageRenderer.AddUniform("z", 0.001);

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
    TextRenderer.AddUniform("z", 0.001);

    TextRenderer.SetProjection(width, height);
    TextRenderer.SetModel();
    TextRenderer.SetView(lookX, lookY);
    TextRenderer.Draw();

  }

}
