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

#include <osmscout/Projection.h>
#include <osmscout/Node.h>
#include <osmscout/Relation.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/Way.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapParameter
  {
  public:
    MapParameter();
    virtual ~MapParameter();
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
  };

  class OSMSCOUT_MAP_API MapPainter
  {
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
      Private draw algortihm implementation routines.
     */
    //@{
    void DrawTiledLabel(const Projection& projection,
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

    void TransformArea(const Projection& projection,
                       const std::vector<Point>& nodes);
    void TransformWay(const Projection& projection,
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
      Draw the complete area in a global preset color (currently the color
      for "normal" background, later on, if we support water/earth detection
      this would be the color for water.
     */
    virtual void ClearArea(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data) = 0;
                                                      
    /**
      Draw the given text at the given pixel coordinate in a style defined
      by the given LabelStyle.
     */
    virtual void DrawLabel(const Projection& projection,
                           const LabelStyle& style,
                           const std::string& text,
                           double x, double y) = 0;

    /**
      Draw the given text as a contour of the given path in a style defined
      by the given LabelStyle.
     */
    virtual void DrawContourLabel(const Projection& projection,
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
      Draw the way using LineStyle for the given type, the given style modification
      attributes and the given path.
     */
    virtual void DrawWay(const StyleConfig& styleConfig,
                         const Projection& projection,
                         TypeId type,
                         double width,
                         bool isBridge,
                         bool isTunnel,
                         const std::vector<Point>& nodes) = 0;
                 
    /**
      Draw the outline of the way using LineStyle for the given type, the given
      style modification attributes and the given path. Also draw sensfull
      line end given that the path has joints with other pathes or not.
     */
    virtual void DrawWayOutline(const StyleConfig& styleConfig,
                                const Projection& projection,
                                TypeId type,
                                double width,
                                bool isBridge,
                                bool isTunnel,
                                bool startIsJoint,
                                bool endIsJoint,
                                const std::vector<Point>& nodes) = 0;

    /**
      Draw the given area, evaluating Fill- and PatternStyle as returned by the
      StyleConfig for the given type. Take into account that Fill- and
      PatternStyle have the layer as passed to the function.
     */                        
    virtual void DrawArea(const StyleConfig& styleConfig,
                          const Projection& projection,
                          TypeId type,
                          int layer,
                          bool isBuilding,
                          const std::vector<Point>& nodes) = 0;
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
