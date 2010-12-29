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

#include <set>

#include <osmscout/Private/MapImportExport.h>

#include <osmscout/GroundTile.h>
#include <osmscout/Node.h>
#include <osmscout/Projection.h>
#include <osmscout/Relation.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/Way.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapParameter
  {
  private:
    std::string fontName;          //! Name of the font to use
    double      fontSize;          //! Pixel size of base font
    double      outlineMinWidth;   //! Minimum width of an outline to be drawn
    bool        optimizeWayNodes;  //! Try to reduce the number of nodes for a way
    bool        optimizeAreaNodes; //! Try to reduce the number of nodes for an area

  public:
    MapParameter();
    virtual ~MapParameter();

    void SetFontName(const std::string& fontName);
    void SetFontSize(double fontSize);

    void SetOutlineMinWidth(double outlineMinWidth);

    void SetOptimizeWayNodes(bool optimize);
    void SetOptimizeAreaNodes(bool optimize);

    inline std::string GetFontName() const
    {
      return fontName;
    }

    inline double GetFontSize() const
    {
      return fontSize;
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
  };

  struct OSMSCOUT_MAP_API MapData
  {
    std::vector<Node>     nodes;
    std::vector<Way>      ways;
    std::vector<Way>      areas;
    std::vector<Relation> relationWays;
    std::vector<Relation> relationAreas;
    std::list<Way>        poiWays;
    std::list<Node>       poiNodes;
    std::list<GroundTile> groundTiles;
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
    /**
       Scratch variables for path optimization algortihm
     */
    //@{
    std::vector<bool>   drawNode;     //! This nodes will be drawn
    std::vector<double> nodeX;        //! static scratch buffer for calculation
    std::vector<double> nodeY;        //! static scratch buffer for calculation
    //@}


    /**
       Style specific precalculations
     */
    //@{
    std::vector<double> borderWidth;  //! border with for this way (area) border style
    bool                areaLayers[11];
    bool                wayLayers[11];
    bool                relationAreaLayers[11];
    bool                relationWayLayers[11];
    //@}

  private:
    /**
      Private draw algorithm implementation routines.
     */
    //@{
    void DrawGroundTiles(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const MapData& data);

    void DrawTiledLabel(const Projection& projection,
                        const MapParameter& parameter,
                        const LabelStyle& style,
                        const std::string& label,
                        const std::vector<Point>& nodes,
                        std::set<size_t>& tileBlacklist);

    void PrecalculateStyleData(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data);

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
                 const std::vector<Point>& nodes);

    /**
      Draw the outline of the way using LineStyle for the given type, the given
      style modification attributes and the given path. Also draw sensfull
      line end given that the path has joints with other pathes or not.
     */
    void DrawWayOutline(const Projection& projection,
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
    void DrawPOINodeLabels(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);
    //@}

  protected:
    /**
       Useful global helper functions.
     */
    //@{
    bool IsVisible(const Projection& projection,
                   const std::vector<Point>& nodes) const;

    bool IsVisible(const Projection& projection,
                   const std::vector<Point>& nodes,
                   double pixelOffset) const;

    void TransformArea(const Projection& projection,
                       const MapParameter& parameter,
                       const std::vector<Point>& nodes);
    void TransformWay(const Projection& projection,
                      const MapParameter& parameter,
                      const std::vector<Point>& nodes);

    bool GetBoundingBox(const std::vector<Point>& nodes,
                        double& xmin, double& ymin,
                        double& xmax, double& ymax);
    bool GetCenterPixel(const Projection& projection,
                        const std::vector<Point>& nodes,
                        double& cx,
                        double& cy);
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
                         IconStyle& style)= 0;

    /**
      Return true, if the pattern in the PatternStyle is available and can be drawn.
      If this method returns false, possibly a fallback (e.g. using a also defined FillStyle)
      will be chosen.
     */
    virtual bool HasPattern(const StyleConfig& styleConfig,
                            PatternStyle& style) = 0;

    /**
      Draw the given text at the given pixel coordinate in a style defined
      by the given LabelStyle.
     */
    virtual void DrawLabel(const Projection& projection,
                           const MapParameter& parameter,
                           const LabelStyle& style,
                           const std::string& text,
                           double x, double y) = 0;

    /**
      Draw the given text as a contour of the given path in a style defined
      by the given LabelStyle.
     */
    virtual void DrawContourLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const LabelStyle& style,
                                  const std::string& text,
                                  const std::vector<Point>& nodes) = 0;

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
    virtual void DrawPath(const LineStyle::Style& style,
                          const Projection& projection,
                          const MapParameter& parameter,
                          double r,
                          double g,
                          double b,
                          double a,
                          double width,
                          CapStyle startCap,
                          CapStyle endCap,
                          const std::vector<Point>& nodes) = 0;

    /**
      Draw the given area using the given FillStyle and (optionally) the given LineStyle
      for the area outline.
     */
    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          TypeId type,
                          const FillStyle& fillStyle,
                          const LineStyle* lineStyle,
                          const std::vector<Point>& nodes) = 0;

    /**
      Draw the given area using the given PatternStyle and (optionally) the given LineStyle
      for the area outline.
     */
    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          TypeId type,
                          const PatternStyle& patternStyle,
                          const LineStyle* lineStyle,
                          const std::vector<Point>& nodes) = 0;

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
