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

#include <osmscout/StyleConfig.h>

#include <set>

#include <iostream>

namespace osmscout {

  LineStyle::LineStyle()
   : lineColor(1.0,0.0,0.0,0.0),
     alternateColor(1,0.0,0.0,0.0),
     outlineColor(1,0.0,0.0,0.0),
     gapColor(1,0.0,0.0,0.0),
     displayWidth(0),
     width(0),
     fixedWidth(false),
     capStyle(capRound),
     outline(0)
  {
    // no code
  }

  LineStyle::LineStyle(const LineStyle& style)
  : lineColor(style.lineColor),
    alternateColor(style.alternateColor),
    outlineColor(style.outlineColor),
    gapColor(style.gapColor),
    displayWidth(style.displayWidth),
    width(style.width),
    fixedWidth(style.fixedWidth),
    capStyle(style.capStyle),
    outline(style.outline),
    dash(style.dash)
  {
    // no code
  }

  LineStyle& LineStyle::SetLineColor(const Color& color)
  {
    this->lineColor=color;

    return *this;
  }

  LineStyle& LineStyle::SetAlternateColor(const Color& color)
  {
    this->alternateColor=color;

    return *this;
  }

  LineStyle& LineStyle::SetOutlineColor(const Color& color)
  {
    outlineColor=color;

    return *this;
  }

  LineStyle& LineStyle::SetGapColor(const Color& color)
  {
    gapColor=color;

    return *this;
  }

  LineStyle& LineStyle::SetDisplayWidth(double value)
  {
    displayWidth=value;

    return *this;
  }

  LineStyle& LineStyle::SetWidth(double value)
  {
    width=value;

    return *this;
  }

  LineStyle& LineStyle::SetFixedWidth(bool fixedWidth)
  {
    this->fixedWidth=fixedWidth;

    return *this;
  }

  LineStyle& LineStyle::SetCapStyle(CapStyle capStyle)
  {
    this->capStyle=capStyle;

    return *this;
  }

  LineStyle& LineStyle::SetOutline(double value)
  {
    outline=value;

    return *this;
  }

  LineStyle& LineStyle::SetDashes(const std::vector<double> dashes)
  {
    dash=dashes;

    return *this;
  }

  void LineStyle::CopyAttributes(const LineStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrLineColor:
        lineColor=other.lineColor;
        break;
      case attrAlternateColor:
        alternateColor=other.alternateColor;
        break;
      case attrOutlineColor:
        outlineColor=other.outlineColor;
        break;
      case attrGapColor:
        gapColor=other.gapColor;
        break;
      case attrDisplayWidth:
        displayWidth=other.displayWidth;
        break;
      case attrWidth:
        width=other.width;
        break;
      case attrFixedWidth:
        fixedWidth=other.fixedWidth;
        break;
      case attrCapStyle:
        capStyle=other.capStyle;
        break;
      case attrOutline:
        outline=other.outline;
        break;
      case attrDashes:
        dash=other.dash;
        break;
      }
    }
  }

  FillStyle::FillStyle()
   : fillColor(1.0,0.0,0.0,0.0),
     patternId(0),
     patternMinMag(magWorld),
     borderColor(1.0,0.0,0.0,0.0),
     borderWidth(0.0)
  {
    // no code
  }

  FillStyle::FillStyle(const FillStyle& style)
  {
    this->fillColor=style.fillColor;
    this->pattern=style.pattern;
    this->patternId=style.patternId;
    this->patternMinMag=style.patternMinMag;
    this->borderColor=style.borderColor;
    this->borderWidth=style.borderWidth;
    this->borderDash=style.borderDash;
  }

  FillStyle& FillStyle::SetFillColor(const Color& color)
  {
    fillColor=color;

    return *this;
  }

  void FillStyle::SetPatternId(size_t id) const
  {
    patternId=id;
  }

  FillStyle& FillStyle::SetPattern(const std::string& pattern)
  {
    this->pattern=pattern;

    return *this;
  }

  FillStyle& FillStyle::SetPatternMinMag(Mag mag)
  {
    patternMinMag=mag;

    return *this;
  }

  FillStyle& FillStyle::SetBorderColor(const Color& color)
  {
    borderColor=color;

    return *this;
  }

  FillStyle& FillStyle::SetBorderWidth(double value)
  {
    borderWidth=value;

    return *this;
  }

  FillStyle& FillStyle::SetBorderDashes(const std::vector<double> dashes)
  {
    borderDash=dashes;

    return *this;
  }

  void FillStyle::CopyAttributes(const FillStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrFillColor:
        fillColor=other.fillColor;
        break;
      case attrPattern:
        pattern=other.pattern;
        break;
      case attrPatternMinMag:
        patternMinMag=other.patternMinMag;
        break;
      case attrBorderColor:
        borderColor=other.borderColor;
        break;
      case attrBorderWidth:
        borderWidth=other.borderWidth;
        break;
      case attrBorderDashes:
        borderDash=other.borderDash;
        break;
      }
    }
  }

  LabelStyle::LabelStyle()
   : priority(0),
     size(1)
  {
    // no code
  }

  LabelStyle::LabelStyle(const LabelStyle& style)
  {
    this->priority=style.priority;
    this->size=style.size;
  }

  LabelStyle::~LabelStyle()
  {
    // no code
  }

  LabelStyle& LabelStyle::SetPriority(uint8_t priority)
  {
    this->priority=priority;

    return *this;
  }

  LabelStyle& LabelStyle::SetSize(double size)
  {
    this->size=size;

    return *this;
  }

  TextStyle::TextStyle()
   : style(normal),
     scaleAndFadeMag((Mag)1000000),
     label(none),
     textColor(0,0,0)

  {
    // no code
  }

  TextStyle::TextStyle(const TextStyle& style)
  : LabelStyle(style),
    style(style.style),
    scaleAndFadeMag(style.scaleAndFadeMag),
    label(style.label),
    textColor(style.textColor)
  {
    // no code
  }

  TextStyle& TextStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  TextStyle& TextStyle::SetPriority(uint8_t priority)
  {
    LabelStyle::SetPriority(priority);

    return *this;
  }

  TextStyle& TextStyle::SetScaleAndFadeMag(Mag mag)
  {
    this->scaleAndFadeMag=mag;

    return *this;
  }

  TextStyle& TextStyle::SetSize(double size)
  {
    LabelStyle::SetSize(size);

    return *this;
  }

  TextStyle& TextStyle::SetLabel(Label label)
  {
    this->label=label;

    return *this;
  }

  TextStyle& TextStyle::SetTextColor(const Color& color)
  {
    this->textColor=color;

    return *this;
  }

  void TextStyle::CopyAttributes(const TextStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrPriority:
        SetPriority(other.GetPriority());
        break;
      case attrSize:
        SetSize(other.GetSize());
        break;
      case attrLabel:
        label=other.label;
        break;
      case attrTextColor:
        textColor=other.textColor;
        break;
      case attrStyle:
        style=other.style;
        break;
      case attrScaleAndFadeMag:
        scaleAndFadeMag=other.scaleAndFadeMag;
        break;
      }
    }
  }

  ShieldStyle::ShieldStyle()
   : label(none),
     textColor(0,0,0),
     bgColor(1,1,1),
     borderColor(0,0,0)
  {
    // no code
  }

  ShieldStyle::ShieldStyle(const ShieldStyle& style)
  : LabelStyle(style)
  {
    this->label=style.label;
    this->textColor=style.textColor;
    this->bgColor=style.bgColor;
    this->borderColor=style.borderColor;
  }

  ShieldStyle& ShieldStyle::SetLabel(Label label)
  {
    this->label=label;

    return *this;
  }

  ShieldStyle& ShieldStyle::SetPriority(uint8_t priority)
  {
    LabelStyle::SetPriority(priority);

    return *this;
  }

  ShieldStyle& ShieldStyle::SetSize(double size)
  {
    LabelStyle::SetSize(size);

    return *this;
  }

  ShieldStyle& ShieldStyle::SetTextColor(const Color& color)
  {
    this->textColor=color;

    return *this;
  }

  ShieldStyle& ShieldStyle::SetBgColor(const Color& color)
  {
    this->bgColor=color;

    return *this;
  }

  ShieldStyle& ShieldStyle::SetBorderColor(const Color& color)
  {
    this->borderColor=color;

    return *this;
  }

  void ShieldStyle::CopyAttributes(const ShieldStyle& other,
                                   const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrPriority:
        SetPriority(other.GetPriority());
        break;
      case attrSize:
        SetSize(other.GetSize());
        break;
      case attrLabel:
        label=other.label;
        break;
      case attrTextColor:
        textColor=other.textColor;
        break;
      case attrBgColor:
        bgColor=other.bgColor;
        break;
      case attrBorderColor:
        borderColor=other.borderColor;
        break;
      }
    }
  }

  PathTextStyle::PathTextStyle()
   : label(none),
     size(1),
     textColor(0,0,0)
  {
    // no code
  }

  PathTextStyle::PathTextStyle(const PathTextStyle& style)
  {
    this->label=style.label;
    this->size=style.size;
    this->textColor=style.textColor;
  }

  PathTextStyle& PathTextStyle::SetLabel(Label label)
  {
    this->label=label;

    return *this;
  }

  PathTextStyle& PathTextStyle::SetSize(double size)
  {
    this->size=size;

    return *this;
  }

  PathTextStyle& PathTextStyle::SetTextColor(const Color& color)
  {
    this->textColor=color;

    return *this;
  }

  void PathTextStyle::CopyAttributes(const PathTextStyle& other,
                                     const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrLabel:
        label=other.label;
        break;
      case attrSize:
        size=other.size;
      case attrTextColor:
        textColor=other.textColor;
        break;
      }
    }
  }

  DrawPrimitive::~DrawPrimitive()
  {
    // no code
  }

  DrawPrimitive::DrawPrimitive(const FillStyleRef& fillStyle)
  : fillStyle(fillStyle)
  {
    // no code
  }

  PolygonPrimitive::PolygonPrimitive(const FillStyleRef& fillStyle)
  : DrawPrimitive(fillStyle)
  {
    // no code
  }

  void PolygonPrimitive::GetBoundingBox(double& minX,
                                        double& minY,
                                        double& maxX,
                                        double& maxY) const
  {
    minX=std::numeric_limits<double>::max();
    minY=std::numeric_limits<double>::max();
    maxX=std::numeric_limits<double>::min();
    maxY=std::numeric_limits<double>::min();

    for (std::list<Pixel>::const_iterator pixel=pixels.begin();
         pixel!=pixels.end();
         ++pixel) {
      minX=std::min(minX,pixel->x);
      minY=std::min(minY,pixel->y);

      maxX=std::max(maxX,pixel->x);
      maxY=std::max(maxY,pixel->y);
    }
  }

  void PolygonPrimitive::AddPixel(const Pixel& pixel)
  {
    pixels.push_back(pixel);
  }

  RectanglePrimitive::RectanglePrimitive(const Pixel& topLeft,
                                         double width,
                                         double height,
                                         const FillStyleRef& fillStyle)
  : DrawPrimitive(fillStyle),
    topLeft(topLeft),
    width(width),
    height(height)
  {
    // no code
  }

  void RectanglePrimitive::GetBoundingBox(double& minX,
                                          double& minY,
                                          double& maxX,
                                          double& maxY) const
  {
    minX=topLeft.x;
    minY=topLeft.y-height;

    maxX=topLeft.x+width;
    maxY=topLeft.y;
  }

  CirclePrimitive::CirclePrimitive(const Pixel& center,
                                   double radius,
                                   const FillStyleRef& fillStyle)
  : DrawPrimitive(fillStyle),
    center(center),
    radius(radius)
  {
    // no code
  }

  void CirclePrimitive::GetBoundingBox(double& minX,
                                       double& minY,
                                       double& maxX,
                                       double& maxY) const
  {
    minX=center.x-radius;
    minY=center.y-radius;

    maxX=center.x+radius;
    maxY=center.y+radius;
  }

  Symbol::Symbol(const std::string& name)
  : name(name),
    minX(std::numeric_limits<double>::max()),
    minY(std::numeric_limits<double>::max()),
    maxX(std::numeric_limits<double>::min()),
    maxY(std::numeric_limits<double>::min())
  {
    // no code
  }

  void Symbol::AddPrimitive(const DrawPrimitiveRef& primitive)
  {
    double minX;
    double minY;
    double maxX;
    double maxY;

    primitive->GetBoundingBox(minX,minY,maxX,maxY);

    this->minX=std::min(this->minX,minX);
    this->minY=std::min(this->minY,minY);

    this->maxX=std::max(this->maxX,maxX);
    this->maxY=std::max(this->maxY,maxY);

    primitives.push_back(primitive);
  }

  IconStyle::IconStyle()
   : id(0)
  {
    // no code
  }

  IconStyle::IconStyle(const IconStyle& style)
  {
    this->id=style.id;
    this->iconName=style.iconName;
  }

  IconStyle& IconStyle::SetSymbol(const SymbolRef& symbol)
  {
    this->symbol=symbol;

    return *this;
  }

  IconStyle& IconStyle::SetId(size_t id)
  {
    this->id=id;

    return *this;
  }

  IconStyle& IconStyle::SetIconName(const std::string& iconName)
  {
    this->iconName=iconName;

    return *this;
  }

  void IconStyle::CopyAttributes(const IconStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrSymbol:
        symbol=other.symbol;
        break;
      case attrIconName:
        iconName=other.iconName;
        break;
      }
    }
  }

  PathSymbolStyle::PathSymbolStyle()
  : symbolSpace(15)
  {
    // no code
  }

  PathSymbolStyle::PathSymbolStyle(const PathSymbolStyle& style)
  : symbol(style.symbol),
    symbolSpace(style.symbolSpace)
  {
    // no code
  }

  PathSymbolStyle& PathSymbolStyle::SetSymbol(const SymbolRef& symbol)
  {
    this->symbol=symbol;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetSymbolSpace(double space)
  {
    symbolSpace=space;

    return *this;
  }

  void PathSymbolStyle::CopyAttributes(const PathSymbolStyle& other,
                                       const std::set<Attribute>& attributes)
  {
    for (std::set<Attribute>::const_iterator a=attributes.begin();
         a!=attributes.end();
         ++a) {
      switch (*a) {
      case attrSymbol:
        symbol=other.symbol;
        break;
      case attrSymbolSpace:
        symbolSpace=other.symbolSpace;
        break;
      }
    }
  }

  StyleFilter::StyleFilter()
  : minLevel(0),
    maxLevel(std::numeric_limits<size_t>::max()),
    oneway(false)
  {
    // no code
  }

  StyleFilter::StyleFilter(const StyleFilter& other)
  {
    this->types=other.types;
    this->minLevel=other.minLevel;
    this->maxLevel=other.maxLevel;
    this->oneway=other.oneway;
  }

  StyleFilter& StyleFilter::SetTypes(const TypeSet& types)
  {
    this->types=types;

    return *this;
  }

  StyleFilter& StyleFilter::SetMinLevel(size_t level)
  {
    minLevel=level;

    return *this;
  }

  StyleFilter& StyleFilter::SetMaxLevel(size_t level)
  {
    maxLevel=level;

    return *this;
  }

  StyleFilter& StyleFilter::SetOneway(bool oneway)
  {
    this->oneway=oneway;

    return *this;
  }

  StyleCriteria::StyleCriteria()
  : minLevel(0),
    maxLevel(std::numeric_limits<size_t>::max()),
    oneway(false)
  {
    // no code
  }

  StyleCriteria::StyleCriteria(const StyleFilter& other)
  {
    this->minLevel=other.GetMinLevel();
    this->maxLevel=other.GetMaxLevel();
    this->oneway=other.GetOneway();
  }

  StyleCriteria::StyleCriteria(const StyleCriteria& other)
  {
    this->minLevel=other.minLevel;
    this->maxLevel=other.maxLevel;
    this->oneway=other.oneway;
  }

  bool StyleCriteria::Matches(size_t level) const
  {
    if (!(level>=minLevel &&
          level<=maxLevel)) {
      return false;
    }

    if (oneway) {
      return false;
    }

    return true;
  }

  bool StyleCriteria::Matches(const SegmentAttributes& attributes,
                              size_t level) const
  {
    if (!(level>=minLevel &&
          level<=maxLevel)) {
      return false;
    }

    if (oneway &&
        !attributes.IsOneway()) {
      return false;
    }

    return true;
  }

  StyleConfig::StyleConfig(TypeConfig* typeConfig)
   : typeConfig(typeConfig)
  {
    wayPrio.resize(typeConfig->GetMaxTypeId()+1,std::numeric_limits<size_t>::max());
  }

  StyleConfig::~StyleConfig()
  {
    // no code
  }

  bool StyleConfig::RegisterSymbol(const SymbolRef& symbol)
  {
    std::pair<OSMSCOUT_HASHMAP<std::string,SymbolRef>::iterator,bool> result;

    result=symbols.insert(std::make_pair(symbol->GetName(),symbol));

    return result.second;
  }

  const SymbolRef& StyleConfig::GetSymbol(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,SymbolRef>::const_iterator entry;

    entry=symbols.find(name);

    if (entry!=symbols.end()) {
      return entry->second;
    }
    else {
      return emptySymbol;
    }
  }

  void StyleConfig::GetAllNodeTypes(std::list<TypeId>& types)
  {
    for (TypeId t=0; t<=typeConfig->GetMaxTypeId(); t++) {
      if (typeConfig->GetTypeInfo(t).CanBeNode()) {
        types.push_back(t);
      }
    }
  }

  void StyleConfig::GetAllWayTypes(std::list<TypeId>& types)
  {
    for (TypeId t=0; t<=typeConfig->GetMaxTypeId(); t++) {
      if (typeConfig->GetTypeInfo(t).CanBeWay()) {
        types.push_back(t);
      }
    }
  }

  void StyleConfig::GetAllAreaTypes(std::list<TypeId>& types)
  {
    for (TypeId t=0; t<=typeConfig->GetMaxTypeId(); t++) {
      if (typeConfig->GetTypeInfo(t).CanBeArea()) {
        types.push_back(t);
      }
    }
  }

  template <class S, class A>
  void GetMaxLevelInConditionals(const std::list<ConditionalStyle<S,A> >& conditionals, size_t& maxLevel)
  {
    for (typename std::list<ConditionalStyle<S,A> >::const_iterator c=conditionals.begin();
         c!=conditionals.end();
         ++c) {
      const ConditionalStyle<S,A>& conditional=*c;

      maxLevel=std::max(maxLevel,conditional.filter.GetMinLevel()+1);

      if (conditional.filter.HasMaxLevel()) {
        maxLevel=std::max(maxLevel,conditional.filter.GetMaxLevel()+1);
      }
    }
  }

  template <class S, class A>
  void CalculateUsedTypes(const TypeConfig typeConfig,
                          const std::list<ConditionalStyle<S,A> >& conditionals,
                          size_t maxLevel,
                          std::vector<TypeSet>& typeSets)
  {
    for (size_t level=0;
        level<maxLevel;
        ++level) {
      for (typename std::list<ConditionalStyle<S,A> >::const_iterator c=conditionals.begin();
           c!=conditionals.end();
           ++c) {
        const ConditionalStyle<S,A>& conditional=*c;

        for (TypeId type=0;
            type<=typeConfig.GetMaxTypeId();
            type++) {
          if (!conditional.filter.HasType(type)) {
            continue;
          }

          if (level<conditional.filter.GetMinLevel()) {
            continue;
          }

          if (conditional.filter.HasMaxLevel() &&
              level>conditional.filter.GetMaxLevel()) {
            continue;
          }

          typeSets[level].SetType(type);
        }
      }
    }
  }

  template <class S, class A>
  void SortInConditionals(const TypeConfig typeConfig,
                          std::list<ConditionalStyle<S,A> >& conditionals,
                          size_t maxLevel,
                          std::vector<std::vector<std::list<StyleSelector<S,A> > > >& selectors)
  {
    selectors.resize(typeConfig.GetMaxTypeId()+1);

    for (TypeId type=0;
        type<=typeConfig.GetMaxTypeId();
        type++) {
      selectors[type].resize(maxLevel+1);
    }

    for (typename std::list<ConditionalStyle<S,A> >::const_iterator conditional=conditionals.begin();
         conditional!=conditionals.end();
         ++conditional) {
      StyleSelector<S,A> selector(conditional->filter,conditional->style);

      for (TypeId type=0;
          type<=typeConfig.GetMaxTypeId();
          type++) {
        if (!conditional->filter.HasType(type)) {
          continue;
        }

        size_t minLvl=conditional->filter.GetMinLevel();
        size_t maxLvl=maxLevel;

        if (conditional->filter.HasMaxLevel()) {
          maxLvl=conditional->filter.GetMaxLevel();
        }
        else {
          maxLvl=maxLevel;
        }

        for (size_t level=minLvl; level<=maxLvl; level++) {
          selectors[type][level].push_back(selector);
        }
      }
    }
  }

  void StyleConfig::PostprocessNodes()
  {
    size_t maxLevel=0;

    GetMaxLevelInConditionals(nodeTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(nodeIconStyleConditionals,
                              maxLevel);

    SortInConditionals(*typeConfig,
                       nodeTextStyleConditionals,
                       maxLevel,
                       nodeTextStyleSelectors);
    SortInConditionals(*typeConfig,
                       nodeIconStyleConditionals,
                       maxLevel,
                       nodeIconStyleSelectors);

    nodeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      nodeTypeSets.push_back(TypeSet(*typeConfig));
    }

    CalculateUsedTypes(*typeConfig,
                       nodeTextStyleConditionals,
                       maxLevel,
                       nodeTypeSets);
    CalculateUsedTypes(*typeConfig,
                       nodeIconStyleConditionals,
                       maxLevel,
                       nodeTypeSets);

    nodeTextStyleConditionals.clear();
    nodeIconStyleConditionals.clear();
  }

  void StyleConfig::PostprocessWays()
  {
    size_t maxLevel=0;

    GetMaxLevelInConditionals(wayLineStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(wayPathTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(wayPathSymbolStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(wayShieldStyleConditionals,
                              maxLevel);

    SortInConditionals(*typeConfig,
                       wayLineStyleConditionals,
                       maxLevel,
                       wayLineStyleSelectors);
    SortInConditionals(*typeConfig,
                       wayPathTextStyleConditionals,
                       maxLevel,
                       wayPathTextStyleSelectors);
    SortInConditionals(*typeConfig,
                       wayPathSymbolStyleConditionals,
                       maxLevel,
                       wayPathSymbolStyleSelectors);
    SortInConditionals(*typeConfig,
                       wayShieldStyleConditionals,
                       maxLevel,
                       wayShieldStyleSelectors);

    wayTypeSets.resize(maxLevel);

    std::set<size_t> prios;

    for (TypeId type=0; type<wayPrio.size(); type++) {
      prios.insert(wayPrio[type]);
    }

    for (size_t level=0;
        level<maxLevel;
        ++level) {
      for (std::set<size_t>::const_iterator prio=prios.begin();
          prio!=prios.end();
          ++prio) {
        TypeSet typeSet(*typeConfig);

        for (TypeId type=0; type<wayPrio.size(); type++) {
          if (!typeConfig->GetTypeInfo(type).CanBeWay()) {
            continue;
          }

          if (wayPrio[type]==*prio) {
            if (!wayLineStyleSelectors[type][level].empty()) {
              typeSet.SetType(type);
            }

            if (!wayPathTextStyleSelectors[type][level].empty()) {
              typeSet.SetType(type);
            }

            if (!wayPathSymbolStyleSelectors[type][level].empty()) {
              typeSet.SetType(type);
            }

            if (!wayShieldStyleSelectors[type][level].empty()) {
              typeSet.SetType(type);
            }
          }
        }

        // TODO: Is type set not empty?
        wayTypeSets[level].push_back(typeSet);
      }
    }

    wayLineStyleConditionals.clear();
    wayPathTextStyleConditionals.clear();
    wayPathSymbolStyleConditionals.clear();
    wayShieldStyleConditionals.clear();
  }

  void StyleConfig::PostprocessAreas()
  {
    size_t maxLevel=0;

    GetMaxLevelInConditionals(areaFillStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaIconStyleConditionals,
                              maxLevel);

    SortInConditionals(*typeConfig,
                       areaFillStyleConditionals,
                       maxLevel,
                       areaFillStyleSelectors);
    SortInConditionals(*typeConfig,
                       areaTextStyleConditionals,
                       maxLevel,
                       areaTextStyleSelectors);
    SortInConditionals(*typeConfig,
                       areaIconStyleConditionals,
                       maxLevel,
                       areaIconStyleSelectors);

    areaTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      areaTypeSets.push_back(TypeSet(*typeConfig));
    }

    CalculateUsedTypes(*typeConfig,
                       areaFillStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaTextStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaIconStyleConditionals,
                       maxLevel,
                       areaTypeSets);

    areaFillStyleConditionals.clear();
    areaTextStyleConditionals.clear();
    areaIconStyleConditionals.clear();
  }

  void StyleConfig::Postprocess()
  {
    PostprocessNodes();
    PostprocessWays();
    PostprocessAreas();
  }

  TypeConfig* StyleConfig::GetTypeConfig() const
  {
    return typeConfig;
  }

  StyleConfig& StyleConfig::SetWayPrio(TypeId type, size_t prio)
  {
    wayPrio[type]=prio;

    return *this;
  }

  void StyleConfig::AddNodeTextStyle(const StyleFilter& filter,
                                     TextPartialStyle& style)
  {
    TextConditionalStyle conditional(filter,style);

    nodeTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddNodeIconStyle(const StyleFilter& filter,
                                        IconPartialStyle& style)
  {
    IconConditionalStyle conditional(filter,style);

    nodeIconStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayLineStyle(const StyleFilter& filter,
                                    LinePartialStyle& style)
  {
    LineConditionalStyle conditional(filter,style);

    wayLineStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayPathTextStyle(const StyleFilter& filter,
                                        PathTextPartialStyle& style)
  {
    PathTextConditionalStyle conditional(filter,style);

    wayPathTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayPathSymbolStyle(const StyleFilter& filter,
                                          PathSymbolPartialStyle& style)
  {
    PathSymbolConditionalStyle conditional(filter,style);

    wayPathSymbolStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayShieldStyle(const StyleFilter& filter,
                                      ShieldPartialStyle& style)
  {
    ShieldConditionalStyle conditional(filter,style);

    wayShieldStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaFillStyle(const StyleFilter& filter,
                                     FillPartialStyle& style)
  {
    FillConditionalStyle conditional(filter,style);

    areaFillStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaTextStyle(const StyleFilter& filter,
                                     TextPartialStyle& style)
  {
    TextConditionalStyle conditional(filter,style);

    areaTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaIconStyle(const StyleFilter& filter,
                                     IconPartialStyle& style)
  {
    IconConditionalStyle conditional(filter,style);

    areaIconStyleConditionals.push_back(conditional);
  }

  void StyleConfig::GetNodeTypesWithMaxMag(double maxMag,
                                           TypeSet& types) const
  {
    if (!nodeTypeSets.empty()) {
      types=nodeTypeSets[std::min(MagToLevel(maxMag),nodeTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetWayTypesByPrioWithMaxMag(double maxMag,
                                                std::vector<TypeSet>& types) const
  {
    if (!wayTypeSets.empty()) {
      types=wayTypeSets[std::min(MagToLevel(maxMag),wayTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetAreaTypesWithMaxMag(double maxMag,
                                           TypeSet& types) const
  {
    if (!areaTypeSets.empty()) {
      types=areaTypeSets[std::min(MagToLevel(maxMag),areaTypeSets.size()-1)];
    }
  }

  template <class S, class A>
  void GetStyle(const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                size_t level,
                Ref<S>& style)
  {
    bool fastpath=style.Invalid();

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (typename std::list<StyleSelector<S,A> >::const_iterator s=styleSelectors[level].begin();
         s!=styleSelectors[level].end();
         ++s) {
      const StyleSelector<S,A>& selector=*s;

      if (!selector.criteria.Matches(level)) {
        continue;
      }

      if (style.Invalid()) {
        style=selector.style;
        continue;
      }
      else if (fastpath) {
        style=new S(selector.style);
        fastpath=false;
      }

      style->CopyAttributes(*selector.style,
                            selector.attributes);
    }
  }

  template <class S, class A>
  void GetNodeStyle(const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                    const Node& node,
                    size_t level,
                    Ref<S>& style)
  {
    bool fastpath=false;

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (typename std::list<StyleSelector<S,A> >::const_iterator s=styleSelectors[level].begin();
         s!=styleSelectors[level].end();
         ++s) {
      const StyleSelector<S,A>& selector=*s;

      if (!selector.criteria.Matches(level)) {
        continue;
      }

      if (style.Invalid()) {
        style=selector.style;
        fastpath=true;

        continue;
      }
      else if (fastpath) {
        style=new S(style);
        fastpath=false;
      }

      style->CopyAttributes(*selector.style,
                            selector.attributes);
    }
  }

  template <class S, class A>
  void GetSegmentAttributesStyle(const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                                 const SegmentAttributes& attributes,
                                 size_t level,
                                 Ref<S>& style)
  {
    bool fastpath=false;

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (typename std::list<StyleSelector<S,A> >::const_iterator s=styleSelectors[level].begin();
         s!=styleSelectors[level].end();
         ++s) {
      const StyleSelector<S,A>& selector=*s;

      if (!selector.criteria.Matches(attributes,level)) {
        continue;
      }

      if (style.Invalid()) {
        style=selector.style;
        fastpath=true;

        continue;
      }
      else if (fastpath) {
        style=new S(style);
        fastpath=false;
      }

      style->CopyAttributes(*selector.style,
                            selector.attributes);
    }
  }

  void StyleConfig::GetNodeTextStyle(const Node& node,
                                     size_t level,
                                     TextStyleRef& textStyle) const
  {
    GetNodeStyle(nodeTextStyleSelectors[node.GetType()],
                 node,
                 level,
                 textStyle);
  }

  void StyleConfig::GetNodeIconStyle(const Node& node,
                                     size_t level,
                                     IconStyleRef& iconStyle) const
  {
    GetNodeStyle(nodeIconStyleSelectors[node.GetType()],
                 node,
                 level,
                 iconStyle);
  }

  void StyleConfig::GetWayLineStyle(const SegmentAttributes& way,
                                    size_t level,
                                    LineStyleRef& lineStyle) const
  {
    GetSegmentAttributesStyle(wayLineStyleSelectors[way.GetType()],
                              way,
                              level,
                              lineStyle);
  }

  void StyleConfig::GetWayPathTextStyle(const SegmentAttributes& way,
                                        size_t level,
                                        PathTextStyleRef& pathTextStyle) const
  {
    GetSegmentAttributesStyle(wayPathTextStyleSelectors[way.GetType()],
                              way,
                              level,
                              pathTextStyle);
  }

  void StyleConfig::GetWayPathSymbolStyle(const SegmentAttributes& way,
                                          size_t level,
                                          PathSymbolStyleRef& pathSymbolStyle) const
  {
    GetSegmentAttributesStyle(wayPathSymbolStyleSelectors[way.GetType()],
                              way,
                              level,
                              pathSymbolStyle);
  }

  void StyleConfig::GetWayShieldStyle(const SegmentAttributes& way,
                                      size_t level,
                                      ShieldStyleRef& shieldStyle) const
  {
    GetSegmentAttributesStyle(wayShieldStyleSelectors[way.GetType()],
                              way,
                              level,
                              shieldStyle);
  }

  void StyleConfig::GetAreaFillStyle(const SegmentAttributes& area,
                                     size_t level,
                                     FillStyleRef& fillStyle) const
  {
    GetSegmentAttributesStyle(areaFillStyleSelectors[area.GetType()],
                              area,
                              level,
                              fillStyle);
  }

  void StyleConfig::GetAreaTextStyle(const SegmentAttributes& area,
                                     size_t level,
                                     TextStyleRef& textStyle) const
  {
    GetSegmentAttributesStyle(areaTextStyleSelectors[area.GetType()],
                              area,
                              level,
                              textStyle);
  }

  void StyleConfig::GetAreaIconStyle(const SegmentAttributes& area,
                                     size_t level,
                                     IconStyleRef& iconStyle) const
  {
    GetSegmentAttributesStyle(areaIconStyleSelectors[area.GetType()],
                              area,
                              level,
                              iconStyle);
  }

  void StyleConfig::GetLandFillStyle(size_t level,
                                     FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileLand],
             level,
             fillStyle);
  }

  void StyleConfig::GetSeaFillStyle(size_t level,
                                    FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileSea],
             level,
             fillStyle);
  }

  void StyleConfig::GetCoastFillStyle(size_t level,
                                      FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileCoast],
             level,
             fillStyle);
  }

  void StyleConfig::GetUnknownFillStyle(size_t level,
                                        FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileUnknown],
             level,
             fillStyle);
  }

  void StyleConfig::GetCoastlineLineStyle(size_t level,
                                       LineStyleRef& lineStyle) const
  {
    GetStyle(wayLineStyleSelectors[typeConfig->typeTileCoastline],
             level,
             lineStyle);
  }
}

