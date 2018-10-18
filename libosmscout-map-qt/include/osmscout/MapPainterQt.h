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

#include <osmscout/MapQtImportExport.h>

#include <osmscout/MapPainter.h>

#include <QtGui/QTextLayout>

namespace osmscout {

  using QtGlyph = Glyph<QGlyphRun>;
  using QtLabel = Label<QGlyphRun, QTextLayout>;
  using QtLabelInstance = LabelInstance<QGlyphRun, QTextLayout>;

  class MapPainterBatchQt;

  /**
    Implementation of MapPainter for Qt
   */
  class OSMSCOUT_MAP_QT_API MapPainterQt : public MapPainter
  {
    friend class MapPainterBatchQt;

    using QtLabelLayouter = LabelLayouter<QGlyphRun, QTextLayout, MapPainterQt>;
    friend QtLabelLayouter;

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
    QPainter                   *painter{nullptr}; //! non-owning pointer to Qt painter

    QtLabelLayouter            labelLayouter;

    /**
     * non-owning pointer to layouter
     * when it is not null, all labels are registered to it
     * and DrawLabels method is no-op
     */
    QtLabelLayouter            *delegateLabelLayouter{nullptr};

    std::map<std::string,QImage> images;        //! map of QImage for icons, key is name of the icon
                                                //! - it should be independent on the specific style configuration
    std::vector<QImage>          patternImages; //! vector of QImage for fill patterns, index is patter id
    std::vector<QBrush>          patterns;      //! vector of QBrush for fill patterns
    QMap<FontDescriptor,QFont>   fonts;         //! Cached fonts
    std::vector<double>          sin;           //! Lookup table for sin calculation

    std::mutex                   mutex;         //! Mutex for locking concurrent calls

  private:
    QFont GetFont(const Projection& projection,
                  const MapParameter& parameter,
                  double fontSize);

    void SetFill(const Projection& projection,
                 const MapParameter& parameter,
                 const FillStyle& fillStyle);

    void SetBorder(const Projection& projection,
                   const MapParameter& parameter,
                   const BorderStyle& borderStyle);

    bool FollowPath(FollowPathHandle &hnd, double l, Vertex2D &origin);
    void FollowPathInit(FollowPathHandle &hnd, Vertex2D &origin, size_t transStart, size_t transEnd,
                        bool isClosed, bool keepOrientation);

    void SetupTransformation(QPainter* painter,
                             const QPointF center,
                             const qreal angle,
                             const qreal baseline) const;

    osmscout::DoubleRectangle GlyphBoundingBox(const QGlyphRun &glyph) const;

    void DrawGlyph(QPainter *painter, const Glyph<QGlyphRun> &glyph) const;

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<osmscout::Glyph<QGlyphRun>> &glyphs);

    std::shared_ptr<QtLabel> Layout(const Projection& projection,
                                    const MapParameter& parameter,
                                    const std::string& text,
                                    double fontSize,
                                    double objectWidth,
                                    bool enableWrapping = false,
                                    bool contourLabel = false);

    QtLabelLayouter& GetLayouter();

  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 IconStyle& style) override;

    bool HasPattern(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style);

    double GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize) override;

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style) override;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const DoubleRectangle& labelRectangle,
                   const LabelData& label,
                   const QTextLayout& textLayout);

    virtual void BeforeDrawing(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data) override;

    /**
      Register regular label with given text at the given pixel coordinate
      in a style defined by the given LabelStyle.
     */
    virtual void RegisterRegularLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const std::vector<LabelData> &labels,
                                      const Vertex2D &position,
                                      double objectWidth) override;

    /**
     * Register contour label
     */
    virtual void RegisterContourLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const PathLabelData &label,
                                      const LabelPath &labelPath) override;

    virtual void DrawLabels(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data) override;

    void DrawIcon(const IconStyle* style,
                  double centerX, double centerY,
                  double width, double height) override;

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

  /**
   * \ingroup Renderer
   *
   * Qt specific MapPainterBatch. When given PainterQt instances
   * are used from multiple threads, they should be always
   * added in same order to avoid deadlocks.
   */
  class OSMSCOUT_MAP_QT_API MapPainterBatchQt:
      public MapPainterBatch<MapPainterQt*> {
  public:
    MapPainterBatchQt(size_t expectedCount);

    virtual ~MapPainterBatchQt();

    bool paint(const Projection& projection,
               const MapParameter& parameter,
               QPainter* painter);
  };
}

#endif
