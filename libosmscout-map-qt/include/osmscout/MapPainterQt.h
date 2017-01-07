#ifndef OSMSCOUT_MAP_MAPPAINTERQT_H
#define OSMSCOUT_MAP_MAPPAINTERQT_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2010  Tim Teulings

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

#include <mutex>

#include <QPainter>

#include <osmscout/private/MapQtImportExport.h>

#include <osmscout/MapPainter.h>

namespace osmscout {

  /**
    Implementation of MapPainter for Qt
   */
  class OSMSCOUT_MAP_QT_API MapPainterQt : public MapPainter
  {
  private:
    struct FollowPathHandle
    {
      bool   closeWay;
      size_t transStart;
      size_t transEnd;
      size_t i;
      size_t nVertex;
      size_t direction;
    };

  private:
    CoordBufferImpl<Vertex2D> *coordBuffer;

    QPainter                  *painter;

    std::vector<QImage>       images;        //! vector of QImage for icons
    std::vector<QImage>       patternImages; //! vector of QImage for fill patterns
    std::vector<QBrush>       patterns;      //! vector of QBrush for fill patterns
    std::map<size_t,QFont>    fonts;         //! Cached fonts
    std::vector<double>       sin;           //! Lookup table for sin calculation

    std::mutex                mutex;         //! Mutex for locking concurrent calls

  private:
    QFont GetFont(const Projection& projection,
                  const MapParameter& parameter,
                  double fontSize);

    void SetPen(const LineStyle& style,
                double lineWidth);

    void SetFill(const Projection& projection,
                 const MapParameter& parameter,
                 const FillStyle& fillStyle);

    bool FollowPath(FollowPathHandle &hnd, double l, Vertex2D &origin);
    void FollowPathInit(FollowPathHandle &hnd, Vertex2D &origin, size_t transStart, size_t transEnd,
                        bool isClosed, bool keepOrientation);

    void setupTransformation(QPainter *painter,
                             const QPointF center,
                             const qreal angle,
                             const qreal baseline) const;

  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style);

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    void GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize,
                       double& height);

    void GetTextDimension(const Projection& projection,
                          const MapParameter& parameter,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height);

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label);

    void DrawIcon(const IconStyle* style,
                  double x, double y);

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x, double y);

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd);

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart, size_t transEnd);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area);

  public:
    MapPainterQt(const StyleConfigRef& styleConfig);
    virtual ~MapPainterQt();


    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 QPainter* painter);
  };
}

#endif
