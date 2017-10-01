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
#include <QMap>

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
    struct FontDescriptor
    {
      QString       fontName;
      size_t        fontSize;
      QFont::Weight weight;
      bool          italic;

      bool operator<(const FontDescriptor& other) const
      {
        if (fontName!=other.fontName)
         return fontName<other.fontName;
        if (fontSize!=other.fontSize)
         return fontSize<other.fontSize;
        if (weight!=other.weight)
         return weight<other.weight;
        return italic<other.italic;
      }
    };

  private:
    QPainter                   *painter;

    std::vector<QImage>        images;        //! vector of QImage for icons
    std::vector<QImage>        patternImages; //! vector of QImage for fill patterns
    std::vector<QBrush>        patterns;      //! vector of QBrush for fill patterns
    QMap<FontDescriptor,QFont> fonts;         //! Cached fonts
    std::vector<double>        sin;           //! Lookup table for sin calculation

    std::mutex                 mutex;         //! Mutex for locking concurrent calls

  private:
    QFont GetFont(const Projection& projection,
                  const MapParameter& parameter,
                  double fontSize);

    void SetPen(const LineStyle& style,
                double lineWidth);

    void SetFill(const Projection& projection,
                 const MapParameter& parameter,
                 const FillStyle& fillStyle);

    void SetBorder(const Projection& projection,
                   const MapParameter& parameter,
                   const BorderStyle& borderStyle);

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
                 IconStyle& style) override;

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    void GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize,
                       double& height) override;

    void GetTextDimension(const Projection& projection,
                          const MapParameter& parameter,
                          double objectWidth,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height) override;

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style) override;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label) override;

    void DrawIcon(const IconStyle* style,
                  double x, double y) override;

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x, double y) override;

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd) override;

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd) override;

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart, size_t transEnd) override;

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area) override;

  public:
    explicit MapPainterQt(const StyleConfigRef& styleConfig);
    ~MapPainterQt() override;

    void DrawGroundTiles(const Projection& projection,
                         const MapParameter& parameter,
                         const std::list<GroundTile>& groundTiles,
                         QPainter* painter);

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 QPainter* painter);
  };
}

#endif
