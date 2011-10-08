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
    double                 dpi;               //! DPI of the display, default is 92

    std::string            fontName;          //! Name of the font to use
    double                 fontSize;          //! Pixel size of base font (aka font size 100%)

    std::list<std::string> iconPaths;         //! List of paths to search for images for icons
    std::list<std::string> patternPaths;      //! List of paths to search for images for patterns

    double                 outlineMinWidth;   //! Minimum width of an outline to be drawn

    double                 drawBridgeMagnification; //! Starting with this magnification, we draw bridges
    double                 drawTunnelMagnification; //! Starting with this magnification, we draw tunnels

    bool                   optimizeWayNodes;  //! Try to reduce the number of nodes for a way
    bool                   optimizeAreaNodes; //! Try to reduce the number of nodes for an area

    bool                   drawFadings;       //! Draw label fadings (default: true)


    bool                   debugPerformance;  //! Print out some performance information

  public:
    MapParameter();
    virtual ~MapParameter();

    void SetDPI(double dpi);

    void SetFontName(const std::string& fontName);
    void SetFontSize(double fontSize);

    void SetIconPaths(const std::list<std::string>& paths);
    void SetPatternPaths(const std::list<std::string>& paths);

    void SetOutlineMinWidth(double outlineMinWidth);

    void SetDrawBridgeMagnification(double magnification);
    void SetDrawTunnelMagnification(double magnification);

    void SetOptimizeWayNodes(bool optimize);
    void SetOptimizeAreaNodes(bool optimize);

    void SetDrawFadings(bool drawFadings);

    void SetDebugPerformance(bool debug);

    inline double GetDPI() const
    {
      return dpi;
    }

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

    inline double GetDrawBridgeMagnification() const
    {
      return drawBridgeMagnification;
    }

    inline double GetDrawTunnelMagnification() const
    {
      return drawTunnelMagnification;
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
    struct OSMSCOUT_API WayData
    {
      const SegmentAttributes *attributes;     //! Attributes of line segment
      const LineStyle         *lineStyle;      //! Line style
      const LabelStyle        *nameLabelStyle; //! LabelStyle for name
      const LabelStyle        *refLabelStyle;  //! LabelStyle for ref
      size_t                  prio;            //! Priority of way (from style sheet)
      size_t                  transStart;      //! Start of coordinates in transformation buffer
      size_t                  transEnd;        //! End of coordinates in transformation buffer
      size_t                  par1Start;       //! Coordinates of upper line (bridge marker)
      size_t                  par1End;         //! Coordinates of upper line (bridge marker)
      size_t                  par2Start;       //! Coordinates of lower line (bridge marker)
      size_t                  par2End;         //! Coordinates of lower line (bridge marker)
      double                  lineWidth;       //! Line width
      double                  outlineWidth;    //! Line width including outline
      bool                    drawBridge;      //! Draw bridge marker
      bool                    drawTunnel;      //! Draw as tunnel
      bool                    outline;         //! Draw outline

      inline bool operator<(const WayData& other)
      {
        if (attributes->GetLayer()==other.attributes->GetLayer())
        {
          return prio>other.prio;
        }
        else {
          return attributes->GetLayer()<other.attributes->GetLayer();
        }
      }
    };

    struct OSMSCOUT_API AreaData
    {
      const SegmentAttributes *attributes;     //! Area attributes
      const FillStyle         *fillStyle;      //! Fill style
      size_t                  transStart;      //! Start of coordinates in transformation buffer
      size_t                  transEnd;        //! End of coordinates in transformation buffer

      inline bool operator<(const AreaData& other)
      {
        return fillStyle->GetLayer()<other.fillStyle->GetLayer();
      }
    };

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
    TransBuffer             transBuffer; //! Static (avoid reallocation) buffer of transformed coordinates
    //@}

    /**
      Presets and similar
     */
    //@{
    std::vector<double>     emptyDash;         //! Empty dash array
    std::vector<double>     tunnelDash;        //! Dash array for drawing tunnel border
    FillStyle               areaMarkStyle;     //! Marker fill style for internal debugging
    //@}

    std::list<AreaData>     areaData;
    std::list<WayData>      wayData;

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
    size_t waysSegments;
    size_t waysDrawn;
    size_t waysOutlineDrawn;
    size_t waysLabelDrawn;

    size_t areasSegments;
    size_t areasDrawn;
    size_t areasLabelDrawn;
    //@}

  private:
    void ScanConvertLine(size_t transStart, size_t transEnd,
                         double cellWidth,
                         double cellHeight,
                         std::vector<ScanCell>& cells);

    /**
      Private draw algorithm implementation routines.
     */
    //@{
    void PrepareAreaSegment(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const SegmentAttributes& attributes,
                            const std::vector<Point>& nodes);

    void PrepareAreas(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void PrepareWaySegment(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const SegmentAttributes& attributes,
                           const std::vector<Point>& nodes);

    void PrepareWays(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const MapData& data);

    void DrawGroundTiles(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const MapData& data);

    void RegisterPointWayLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const LabelStyle& style,
                               const std::string& text,
                               size_t transStart, size_t transEnd);

    bool RegisterPointLabel(const Projection& projection,
                            const MapParameter& parameter,
                            const LabelStyle& style,
                            const std::string& text,
                            double x,
                            double y);



    void DrawWayLabels(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const MapParameter& parameter,
                       const MapData& data);

    void DrawAreaLabels(const StyleConfig& styleConfig,
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
                                  size_t transStart, size_t transEnd) = 0;

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
                          size_t transStart, size_t transEnd) = 0;

    /**
      Draw the given area using the given FillStyle
      for the area outline.
     */
    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          const AreaData& area) = 0;

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

    /**
      Med level drawing routines that are already implemented by the base class, but which can be overwritten
      by the driver if necessary.
     */
    //@{
    virtual void DrawNodes(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);

    /**
      Draw the outline of the way using LineStyle for the given type, the given
      style modification attributes and the given path. Also draw sensfull
      line end given that the path has joints with other pathes or not.
     */
    virtual void DrawWayOutline(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const WayData& data);

    virtual void DrawWay(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const WayData& data);

    virtual void DrawWays(const StyleConfig& styleConfig,
                          const Projection& projection,
                          const MapParameter& parameter,
                          const MapData& data);

    virtual void DrawAreas(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);

    void Draw(const StyleConfig& styleConfig,
              const Projection& projection,
              const MapParameter& parameter,
              const MapData& data);
    //@}

  public:
    MapPainter();
    virtual ~MapPainter();
  };
}

#endif
