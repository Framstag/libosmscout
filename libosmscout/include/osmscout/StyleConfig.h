#ifndef OSMSCOUT_STYLECONFIG_H
#define OSMSCOUT_STYLECONFIG_H

/*
  This source is part of the libosmscout library
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
#include <vector>

#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
   * Ways can have a line style
   */
  class OSMSCOUT_API LineStyle
  {
  private:
    double              lineR;
    double              lineG;
    double              lineB;
    double              lineA;
    double              alternateR;
    double              alternateG;
    double              alternateB;
    double              alternateA;
    double              outlineR;
    double              outlineG;
    double              outlineB;
    double              outlineA;
    double              minPixel;
    double              width;
    double              fixedWidth;
    double              outline;
    std::vector<double> dash;

  public:
    LineStyle();

    LineStyle& SetLineColor(double r, double g, double b, double a);
    LineStyle& SetAlternateColor(double r, double g, double b, double a);
    LineStyle& SetOutlineColor(double r, double g, double b, double a);
    LineStyle& SetMinPixel(double value);
    LineStyle& SetWidth(double value);
    LineStyle& SetFixedWidth(bool fixedWidth);
    LineStyle& SetOutline(double value);
    LineStyle& AddDashValue(double dashValue);

    inline bool IsVisible() const
    {
      return width>0.0;
    }

    inline double GetLineR() const
    {
      return lineR;
    }

    inline double GetLineG() const
    {
      return lineG;
    }

    inline double GetLineB() const
    {
      return lineB;
    }

    inline double GetLineA() const
    {
      return lineA;
    }

    inline double GetAlternateR() const
    {
      return alternateR;
    }

    inline double GetAlternateG() const
    {
      return alternateG;
    }

    inline double GetAlternateB() const
    {
      return alternateB;
    }

    inline double GetAlternateA() const
    {
      return alternateA;
    }

    inline double GetOutlineR() const
    {
      return outlineR;
    }

    inline double GetOutlineG() const
    {
      return outlineG;
    }

    inline double GetOutlineB() const
    {
      return outlineB;
    }

    inline double GetOutlineA() const
    {
      return outlineA;
    }

    inline double GetMinPixel() const
    {
      return minPixel;
    }

    inline double GetWidth() const
    {
      return width;
    }

    inline bool GetFixedWidth() const
    {
      return fixedWidth;
    }

    inline double GetOutline() const
    {
      return outline;
    }

    inline bool HasDashValues() const
    {
      return dash.size()>0;
    }

    inline const std::vector<double>& GetDash() const
    {
      return dash;
    }
  };

  /**
   * Areas can have a fill style, filling the area with one color
   */
  class OSMSCOUT_API FillStyle
  {
  public:
    enum Style {
      none,
      plain
    };

  private:
    Style       style;
    int         layer;
    double      fillR;
    double      fillG;
    double      fillB;
    double      fillA;

  public:
    FillStyle();

    FillStyle& SetStyle(Style style);
    FillStyle& SetLayer(int layer);
    FillStyle& SetColor(double r, double g, double b, double a);

    inline bool IsVisible() const
    {
      return style!=none;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline int GetLayer() const
    {
      return layer;
    }

    inline double GetFillR() const
    {
      return fillR;
    }

    inline double GetFillG() const
    {
      return fillG;
    }

    inline double GetFillB() const
    {
      return fillB;
    }
  };

  /**
   * Areas can have a pattern style, filling the area with an repeated image pattern
   */
  class OSMSCOUT_API PatternStyle
  {
  private:
    int         layer;
    size_t      id;
    std::string pattern;
    Mag         minMag;

  public:
    PatternStyle();

    PatternStyle& SetLayer(int layer);
    PatternStyle& SetId(size_t id);
    PatternStyle& SetPattern(const std::string& pattern);
    PatternStyle& SetMinMag(Mag mag);

    inline int GetLayer() const
    {
      return layer;
    }

    inline size_t GetId() const
    {
      return id;
    }

    inline std::string GetPatternName() const
    {
      return pattern;
    }

    inline const Mag& GetMinMag() const
    {
      return minMag;
    }
  };

  /**
    Nodes, ways and areas can have a label style for drawing text. Text can be formatted
    in different ways.
   */
  class OSMSCOUT_API LabelStyle
  {
  public:
    enum Style {
      none,
      normal,
      contour,
      plate,
      emphasize
    };

  private:
    Style style;
    Mag   minMag;
    Mag   scaleAndFadeMag;
    Mag   maxMag;
    double size;
    double textR;
    double textG;
    double textB;
    double textA;
    double bgR;
    double bgG;
    double bgB;
    double bgA;
    double borderR;
    double borderG;
    double borderB;
    double borderA;

  public:
    LabelStyle();

    LabelStyle& SetStyle(Style style);
    LabelStyle& SetMinMag(Mag mag);
    LabelStyle& SetScaleAndFadeMag(Mag mag);
    LabelStyle& SetMaxMag(Mag mag);
    LabelStyle& SetSize(double size);
    LabelStyle& SetTextColor(double r, double g, double b, double a);
    LabelStyle& SetBgColor(double r, double g, double b, double a);
    LabelStyle& SetBorderColor(double r, double g, double b, double a);

    inline bool IsVisible() const
    {
      return style!=none;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline const Mag& GetMinMag() const
    {
      return minMag;
    }

    inline const Mag& GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    inline const Mag& GetMaxMag() const
    {
      return maxMag;
    }

    inline double GetSize() const
    {
      return size;
    }

    inline double GetTextR() const
    {
      return textR;
    }

    inline double GetTextG() const
    {
      return textG;
    }

    inline double GetTextB() const
    {
      return textB;
    }

    inline double GetTextA() const
    {
      return textA;
    }

    inline double GetBgR() const
    {
      return bgR;
    }

    inline double GetBgG() const
    {
      return bgG;
    }

    inline double GetBgB() const
    {
      return bgB;
    }

    inline double GetBgA() const
    {
      return bgA;
    }

    inline double GetBorderR() const
    {
      return borderR;
    }

    inline double GetBorderG() const
    {
      return borderG;
    }

    inline double GetBorderB() const
    {
      return borderB;
    }

    inline double GetBorderA() const
    {
      return borderA;
    }
  };

  /**
    Nodes and areas can have a symbol style.A symbol is a internal predefined simple
    iconic image, most of the time simple geometric forms lice circles, crosses and
    similar.
   */
  class OSMSCOUT_API SymbolStyle
  {
  public:
    enum Style {
      none,
      box,
      circle,
      triangle
    };

  private:
    Style  style;
    Mag    minMag;
    double size;
    double fillR;
    double fillG;
    double fillB;
    double fillA;

  public:
    SymbolStyle();

    SymbolStyle& SetStyle(Style style);
    SymbolStyle& SetMinMag(Mag mag);
    SymbolStyle& SetSize(double size);
    SymbolStyle& SetFillColor(double r, double g, double b, double a);

    inline bool IsVisible() const
    {
      return style!=none;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline const Mag& GetMinMag() const
    {
      return minMag;
    }

    inline double GetSize() const
    {
      return size;
    }

    inline double GetFillR() const
    {
      return fillR;
    }

    inline double GetFillG() const
    {
      return fillG;
    }

    inline double GetFillB() const
    {
      return fillB;
    }

    inline double GetFillA() const
    {
      return fillA;
    }
  };

  /**
    IconStyle is for define drawing of external images as icons for nodes and areas
    */
  class OSMSCOUT_API IconStyle
  {
  private:
    size_t      id;       //! Internal id for fast lookup. 0 == no id defined (yet), max(size_t) == error
    std::string iconName; //! name of the icon as given in style
    Mag         minMag;   //! minimum magnification to show icon

  public:
    IconStyle();

    IconStyle& SetId(size_t id);
    IconStyle& SetIconName(const std::string& iconName);
    IconStyle& SetMinMag(Mag mag);

    inline bool IsVisible() const
    {
      return !iconName.empty();
    }

    inline size_t GetId() const
    {
      return id;
    }

    inline std::string GetIconName() const
    {
      return iconName;
    }

    inline const Mag& GetMinMag() const
    {
      return minMag;
    }
  };

  /**
   * A complete style definition
   */
  class OSMSCOUT_API StyleConfig
  {
  private:
    TypeConfig                *typeConfig;

    // Node

    std::vector<SymbolStyle*>  nodeSymbolStyles;
    std::vector<LabelStyle*>   nodeRefLabelStyles;
    std::vector<LabelStyle*>   nodeLabelStyles;
    std::vector<IconStyle*>    nodeIconStyles;

    // Way

    std::vector<LineStyle*>    wayLineStyles;
    std::vector<LabelStyle*>   wayRefLabelStyles;
    std::vector<LabelStyle*>   wayNameLabelStyles;

    // Area

    std::vector<FillStyle*>    areaFillStyles;
    std::vector<FillStyle*>    areaBuildingFillStyles;
    std::vector<PatternStyle*> areaPatternStyles;
    std::vector<SymbolStyle*>  areaSymbolStyles;
    std::vector<LabelStyle*>   areaLabelStyles;
    std::vector<LineStyle*>    areaBorderStyles;
    std::vector<IconStyle*>    areaIconStyles;

    std::vector<size_t>        wayPrio;
    std::vector<size_t>        priorities;
    std::vector<TypeId>        wayTypes;

  public:
    StyleConfig(TypeConfig* typeConfig);
    virtual ~StyleConfig();

    void Postprocess();

    TypeConfig* GetTypeConfig() const;

    StyleConfig& SetWayPrio(TypeId type, size_t prio);

    StyleConfig& SetNodeSymbolStyle(TypeId type, const SymbolStyle& style);
    StyleConfig& SetNodeRefLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetNodeLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetNodeIconStyle(TypeId type, const IconStyle& style);

    StyleConfig& SetWayLineStyle(TypeId type, const LineStyle& style);
    StyleConfig& SetWayRefLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetWayNameLabelStyle(TypeId type, const LabelStyle& style);

    StyleConfig& SetAreaFillStyle(TypeId type, const FillStyle& style);
    StyleConfig& SetAreaBuildingFillStyle(TypeId type, const FillStyle& style);
    StyleConfig& SetAreaPatternStyle(TypeId type, const PatternStyle& style);
    StyleConfig& SetAreaLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetAreaSymbolStyle(TypeId type, const SymbolStyle& style);
    StyleConfig& SetAreaBorderStyle(TypeId type, const LineStyle& style);
    StyleConfig& SetAreaIconStyle(TypeId type, const IconStyle& style);

    size_t GetStyleCount() const;

    void GetWayTypesByPrio(std::vector<TypeId>& types) const;
    void GetWayTypesWithMaxPrio(size_t prio, std::set<TypeId>& types) const;
    void GetNodeTypesWithMag(double mag, std::vector<TypeId>& types) const;
    void GetPriorities(std::vector<size_t>& priorities) const;

    bool IsWayVisible(TypeId type, size_t prio) const
    {
      if (type<wayLineStyles.size() && type<wayPrio.size()) {
        return wayLineStyles[type]!=NULL && wayPrio[type]<=prio;
      }
      else {
        return false;
      }
    }

    bool IsAreaVisible(TypeId type, size_t prio) const
    {
      if (type<areaFillStyles.size()) {
        return areaFillStyles[type]!=NULL;
      }
      else if (type<areaPatternStyles.size()) {
        return areaPatternStyles[type]!=NULL;
      }
      else {
        return false;
      }
    }

    bool IsNodeVisible(TypeId type, double mag) const
    {
      if (type<nodeSymbolStyles.size() &&
          nodeSymbolStyles[type]!=NULL &&
          nodeSymbolStyles[type]->GetMinMag()<=mag) {
        return true;
      }
      else if (type<nodeLabelStyles.size() &&
               nodeLabelStyles[type]!=NULL &&
               nodeLabelStyles[type]->GetMinMag()<=mag) {
        return true;
      }

      return false;
    }

    inline const SymbolStyle* GetNodeSymbolStyle(TypeId type) const
    {
      if (type<nodeSymbolStyles.size()) {
        return nodeSymbolStyles[type];
      }
      else {
        return NULL;
      }
    }

    inline IconStyle* GetNodeIconStyle(TypeId type) const
    {
      if (type<nodeIconStyles.size()) {
        return nodeIconStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetNodeRefLabelStyle(TypeId type) const
    {
      if (type<nodeRefLabelStyles.size()) {
        return nodeRefLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetNodeLabelStyle(TypeId type) const
    {
      if (type<nodeLabelStyles.size()) {
        return nodeLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LineStyle* GetWayLineStyle(TypeId type) const
    {
      if (type<wayLineStyles.size()) {
        return wayLineStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetWayRefLabelStyle(TypeId type) const
    {
      if (type<wayRefLabelStyles.size()) {
        return wayRefLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetWayNameLabelStyle(TypeId type) const
    {
      if (type<wayNameLabelStyles.size()) {
        return wayNameLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const FillStyle* GetAreaFillStyle(TypeId type, bool building) const
    {
      if (type<areaFillStyles.size()) {
        if (building && areaBuildingFillStyles[type]!=NULL) {
          return areaBuildingFillStyles[type];
        }
        else {
          return areaFillStyles[type];
        }
      }
      else {
        return NULL;
      }
    }

    PatternStyle* GetAreaPatternStyle(TypeId type) const
    {
      if (type<areaPatternStyles.size()) {
        return areaPatternStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetAreaLabelStyle(TypeId type) const
    {
      if (type<areaLabelStyles.size()) {
        return areaLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const SymbolStyle* GetAreaSymbolStyle(TypeId type) const
    {
      if (type<areaSymbolStyles.size()) {
        return areaSymbolStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LineStyle* GetAreaBorderStyle(TypeId type) const
    {
      if (type<areaBorderStyles.size()) {
        return areaBorderStyles[type];
      }
      else {
        return NULL;
      }
    }

    inline IconStyle* GetAreaIconStyle(TypeId type) const
    {
      if (type<areaIconStyles.size()) {
        return areaIconStyles[type];
      }
      else {
        return NULL;
      }
    }

  };
}

#endif
