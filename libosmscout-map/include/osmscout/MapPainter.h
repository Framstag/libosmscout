#ifndef OSMSCOUT_MAP_MAPPAINTER_H
#define OSMSCOUT_MAP_MAPPAINTER_H

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

#include <list>
#include <set>
#include <string>

#include <osmscout/Private/MapImportExport.h>

#include <osmscout/util/Geometry.h>

#include <osmscout/GroundTile.h>
#include <osmscout/Node.h>
#include <osmscout/Relation.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/Way.h>

#include <osmscout/util/Projection.h>
#include <osmscout/util/Transformation.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapParameter
  {
  private:
    std::string            fontName;          //! Name of the font to use
    double                 fontSize;          //! Pixel size of base font (aka font size 100%)
    std::list<std::string> iconPaths;         //! List of paths to search for images for icons
    std::list<std::string> patternPaths;      //! List of paths to search for images for patterns

    double                 outlineMinWidth;   //! Minimum width of an outline to be drawn

    bool                   optimizeWayNodes;  //! Try to reduce the number of nodes for a way
    bool                   optimizeAreaNodes; //! Try to reduce the number of nodes for an area

    bool                   drawFadings;       //! Draw label fadings (default: true)

    bool                   debugPerformance;  //! Print out some performance information

  public:
    MapParameter();
    virtual ~MapParameter();

    void SetFontName(const std::string& fontName);
    void SetFontSize(double fontSize);

    void SetIconPaths(const std::list<std::string>& paths);
    void SetPatternPaths(const std::list<std::string>& paths);

    void SetOutlineMinWidth(double outlineMinWidth);

    void SetOptimizeWayNodes(bool optimize);
    void SetOptimizeAreaNodes(bool optimize);

    void SetDrawFadings(bool drawFadings);

    void SetDebugPerformance(bool debug);

    inline std::string GetFontName() const
    {
      return fontName;
    }

    inline double GetFontSize() const
    {
      return fontSize;
    }

    inline const std::list<std::string>& GetIconPaths() const
    {
      return iconPaths;
    }

    inline const std::list<std::string>& GetPatternPaths() const
    {
      return patternPaths;
    }

    inline double GetOutlineMinWidth() const
    {
      return outlineMinWidth;
    }

    inline bool GetOptimizeWayNodes() const
    {
      return optimizeWayNodes;
    }

    inline bool GetOptimizeAreaNodes() const
    {
      return optimizeAreaNodes;
    }

    inline bool GetDrawFadings() const
    {
      return drawFadings;
    }

    inline bool IsDebugPerformance() const
    {
      return debugPerformance;
    }
  };

  struct OSMSCOUT_MAP_API MapData
  {
    std::vector<NodeRef>     nodes;
    std::vector<WayRef>      ways;
    std::vector<WayRef>      areas;
    std::vector<RelationRef> relationWays;
    std::vector<RelationRef> relationAreas;
    std::list<WayRef>        poiWays;
    std::list<NodeRef>       poiNodes;
    std::list<GroundTile>    groundTiles;
  };

  class OSMSCOUT_MAP_API MapPainter
  {
  public:
    enum CapStyle
    {
      capButt,
      capRound
    };

  protected:
    struct OSMSCOUT_API Label
    {
      bool              draw;
      bool              overlay;
      bool              mark;
      double            x;
      double            y;
      double            width;
      double            height;
      double            bx;
      double            by;
      double            bwidth;
      double            bheight;
      double            alpha;
      double            fontSize;
      const LabelStyle* style;
      std::string       text;
    };

  protected:
    /**
       Scratch variables for path optimization algorithm
     */
    //@{
    TransPolygon            polygon;  //! Static (avoid reallocation) transformed polygon
    //@}

    /**
       Style specific precalculations
     */
    //@{
    bool                    areaLayers[11];
    bool                    wayLayers[11];
    bool                    relationAreaLayers[11];
    bool                    relationWayLayers[11];
    //@}

    /**
      Presets and similar
     */
    //@{
    std::vector<double>     emptyDash;         //! Empty dash array
    std::vector<double>     tunnelDash;        //! Dash array for drawing tunnel border
    FillStyle               areaMarkStyle;     //! Marker fill style for internal debugging
    //@}

    /**
      Temporary data structures for intelligent label positioning
      */
    //@{
    int                     xCellCount;
    int                     yCellCount;
    size_t                  cellWidth;
    size_t                  cellHeight;
    std::vector<std::list<size_t> > labelRefs;
    std::vector<Label>      labels;
    std::vector<ScanCell>   wayScanlines;
    //@}

    /**
      Statistics counter
     */
    //@{
    size_t waysCount;
    size_t waysDrawn;
    size_t waysOutlineDrawn;
    size_t waysLabelDrawn;

    size_t relWaysCount;
    size_t relWaysDrawn;
    size_t relWaysOutlineDrawn;
    size_t relWaysLabelDrawn;

    size_t areasCount;
    size_t areasDrawn;
    size_t areasLabelDrawn;

    size_t relAreasCount;
    size_t relAreasDrawn;
    size_t relAreasLabelDrawn;
    //@}

  private:
    void ScanConvertLine(const TransPolygon& polygon,
                         double cellWidth,
                         double cellHeight,
                         std::vector<ScanCell>& cells);

    /**
      Private draw algorithm implementation routines.
     */
    //@{
    void DrawGroundTiles(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const MapData& data);

    void PrecalculateStyleData(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data);

    void RegisterPointWayLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const LabelStyle& style,
                               const std::string& text,
                               const TransPolygon& polygon);

    bool RegisterPointLabel(const Projection& projection,
                            const MapParameter& parameter,
                            const LabelStyle& style,
                            const std::string& text,
                            double x,
                            double y);


    void DrawNodes(const StyleConfig& styleConfig,
                   const Projection& projection,
                   const MapParameter& parameter,
                   const MapData& data);

    /**
      Draw the way using LineStyle for the given type, the given style modification
      attributes and the given path.
     */
    void DrawWay(const Projection& projection,
                 const MapParameter& parameter,
                 const LineStyle& style,
                 const SegmentAttributes& attributes,
                 const TransPolygon& polygon);

    /**
      Draw the outline of the way using LineStyle for the given type, the given
      style modification attributes and the given path. Also draw sensfull
      line end given that the path has joints with other pathes or not.
     */
    bool DrawWayOutline(const Projection& projection,
                        const MapParameter& parameter,
                        const LineStyle& style,
                        const SegmentAttributes& attributes,
                        const std::vector<Point>& nodes);

    void DrawWays(const StyleConfig& styleConfig,
                  const Projection& projection,
                  const MapParameter& parameter,
                  const MapData& data);

    void DrawWayLabels(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const MapParameter& parameter,
                       const MapData& data);

    void DrawAreas(const StyleConfig& styleConfig,
                   const Projection& projection,
                   const MapParameter& parameter,
                   const MapData& data);

    void DrawAreaLabels(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data);

    void DrawPOIWays(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const MapData& data);
    void DrawPOINodes(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void DrawLabels(const StyleConfig& styleConfig,
                    const Projection& projection,
                    const MapParameter& parameter);
    //@}

  protected:
    /**
       Useful global helper functions.
     */
    //@{
    bool IsVisible(const Projection& projection,
                   const std::vector<Point>& nodes,
                   double pixelOffset) const;

    void Transform(const Projection& projection,
                   const MapParameter& parameter,
                   double lon,
                   double lat,
                   double& x,
                   double& y);

    bool GetBoundingBox(const std::vector<Point>& nodes,
                        double& xmin, double& ymin,
                        double& xmax, double& ymax) const;
    bool GetCenterPixel(const Projection& projection,
                        const std::vector<Point>& nodes,
                        double& cx,
                        double& cy) const;

    double GetProjectedWidth(const Projection& projection,
                             double minPixel,
                             double width) const;
    //@}

    /**
      Low level drawing routines that have to be implemented by
      the concrete drawing engine.
     */
    //@{

    /**
      Return true, if the icon in the IconStyle is available and can be drawn.
      If this method returns false, possibly a fallback (using a Symbol)
      will be chosen.
     */
    virtual bool HasIcon(const StyleConfig& styleConfig,
                         const MapParameter& parameter,
                         IconStyle& style)= 0;

    /**
      Return true, if the pattern in the PatternStyle is available and can be drawn.
      If this method returns false, possibly a fallback (e.g. using a also defined FillStyle)
      will be chosen.
     */
    virtual bool HasPattern(const StyleConfig& styleConfig,
                            const MapParameter& parameter,
                            PatternStyle& style) = 0;

    /**
      Return the bounding box of the given text.

      The backend may decide to relayout the given text, however it must assure
      that later calls to corresponding DrawXXX methods will honour the initial
      bounding box.
      */
    virtual void GetTextDimension(const MapParameter& parameter,
                                  double fontSize,
                                  const std::string& text,
                                  double& xOff,
                                  double& yOff,
                                  double& width,
                                  double& height) = 0;

    /**
      Draw the given text at the given pixel coordinate in a style defined
      by the given LabelStyle.
     */
    virtual void DrawLabel(const Projection& projection,
                           const MapParameter& parameter,
                           const Label& label) = 0;

    /**
      Draw the given text at the given pixel coordinate in a style defined
      by the given LabelStyle. Draw a plate style icon around the text.
     */
    virtual void DrawPlateLabel(const Projection& projection,
                                const MapParameter& parameter,
                                const Label& label) = 0;

    /**
      Draw the given text as a contour of the given path in a style defined
      by the given LabelStyle.
     */
    virtual void DrawContourLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const LabelStyle& style,
                                  const std::string& text,
                                  const TransPolygon& polygon) = 0;

    /**
      Draw the Icon as defined by the IconStyle at the givcen pixel coordinate.
     */
    virtual void DrawIcon(const IconStyle* style,
                          double x, double y) = 0;

    /**
      Draw the Symbol as defined by the SymbolStyle at the givcen pixel coordinate.
     */
    virtual void DrawSymbol(const SymbolStyle* style,
                            double x, double y) = 0;

    /**
      Draw simple line with the given style,the given color, the given width
      and the given untransformed nodes.
     */
    virtual void DrawPath(const Projection& projection,
                          const MapParameter& parameter,
                          double r, double g, double b, double a,
                          double width,
                          const std::vector<double>& dash,
                          CapStyle startCap,
                          CapStyle endCap,
                          const TransPolygon& polygon) = 0;

    /**
      Draw the given area using the given FillStyle
      for the area outline.
     */
    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          TypeId type,
                          const FillStyle& fillStyle,
                          const TransPolygon& polygon) = 0;

    /**
      Draw the given area using the given PatternStyle
      for the area outline.
     */
    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          TypeId type,
                          const PatternStyle& patternStyle,
                          const TransPolygon& polygon) = 0;

    /**
      Draw the given area in using the given fill. This is currently done to
      complete fill the map area with ground color,
      later on, if we support water/earth detection it would be used for drawing
      water and ground tiles.
     */
    virtual void DrawArea(const FillStyle& style,
                          const MapParameter& parameter,
                          double x,
                          double y,
                          double width,
                          double height) = 0;

    //@}

    void Draw(const StyleConfig& styleConfig,
              const Projection& projection,
              const MapParameter& parameter,
              const MapData& data);

  public:
    MapPainter();
    virtual ~MapPainter();
  };
}

#endif
