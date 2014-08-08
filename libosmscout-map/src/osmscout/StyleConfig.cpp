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

#include <string.h>

#include <set>

#include <iostream>
#include <sstream>

#include <osmscout/system/Assert.h>

#include <osmscout/util/File.h>

#include <osmscout/oss/Parser.h>
#include <osmscout/oss/Scanner.h>

namespace osmscout {

  StyleVariable::StyleVariable()
  {
    // no code
  }

  StyleVariable::~StyleVariable()
  {
    // no code
  }

  StyleVariableColor::StyleVariableColor(const Color& color)
  : color(color)
  {
    // no code
  }

  StyleVariableMag::StyleVariableMag(Magnification& magnification)
  : magnification(magnification)
  {
    // no code
  }

  StyleVariableUInt::StyleVariableUInt(size_t& value)
  : value(value)
  {
    // no code
  }

  SizeCondition::SizeCondition()
  : minMM(0.0),
    minMMSet(false),
    minPx(0.0),
    minPxSet(false),
    maxMM(0.0),
    maxMMSet(false),
    maxPx(0.0),
    maxPxSet(false)
  {
    // no code
  }

  SizeCondition::~SizeCondition()
  {
    // no code
  }

  void SizeCondition::SetMinMM(double minMM)
  {
    this->minMM=minMM;
    this->minMMSet=true;
  }

  void SizeCondition::SetMinPx(double minPx)
  {
    this->minPx=minPx;
    this->minPxSet=true;
  }

  void SizeCondition::SetMaxMM(double maxMM)
  {
    this->maxMM=maxMM;
    this->maxMMSet=true;
  }

  void SizeCondition::SetMaxPx(double maxPx)
  {
    this->maxPx=maxPx;
    this->maxPxSet=true;
  }

  bool SizeCondition::Evaluate(double meterInPixel,
                               double meterInMM) const
  {
    bool matchesMinMM;
    bool matchesMinPx;

    if (minMMSet) {
      matchesMinMM=meterInMM>=minMM;
    }
    else {
      matchesMinMM=true;
    }

    if (minPxSet) {
      matchesMinPx=meterInPixel>=minPx;
    }
    else {
      matchesMinPx=true;
    }

    if (!matchesMinMM || !matchesMinPx) {
      return false;
    }

    bool matchesMaxMM;
    bool matchesMaxPx;

    if (maxMMSet) {
      matchesMaxMM=meterInMM<maxMM;
    }
    else {
      matchesMaxMM=true;
    }

    if (maxPxSet) {
      matchesMaxPx=meterInPixel<maxPx;
    }
    else {
      matchesMaxPx=true;
    }

    if (!matchesMaxMM && !matchesMaxPx) {
      return false;
    }

    return true;
  }

  LineStyle::LineStyle()
   : lineColor(1.0,0.0,0.0,0.0),
     gapColor(1,0.0,0.0,0.0),
     displayWidth(0.0),
     width(0.0),
     displayOffset(0.0),
     offset(0.0),
     joinCap(capRound),
     endCap(capRound),
     priority(0)
  {
    // no code
  }

  LineStyle::LineStyle(const LineStyle& style)
  : slot(style.slot),
    lineColor(style.lineColor),
    gapColor(style.gapColor),
    displayWidth(style.displayWidth),
    width(style.width),
    displayOffset(style.displayOffset),
    offset(style.offset),
    joinCap(style.joinCap),
    endCap(style.endCap),
    dash(style.dash),
    priority(style.priority)
  {
    // no code
  }

  LineStyle& LineStyle::SetSlot(const std::string& slot)
  {
    this->slot=slot;

    return *this;
  }

  LineStyle& LineStyle::SetLineColor(const Color& color)
  {
    this->lineColor=color;

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

  LineStyle& LineStyle::SetDisplayOffset(double value)
  {
    displayOffset=value;

    return *this;
  }

  LineStyle& LineStyle::SetOffset(double value)
  {
    offset=value;

    return *this;
  }

  LineStyle& LineStyle::SetJoinCap(CapStyle joinCap)
  {
    this->joinCap=joinCap;

    return *this;
  }

  LineStyle& LineStyle::SetEndCap(CapStyle endCap)
  {
    this->endCap=endCap;

    return *this;
  }

  LineStyle& LineStyle::SetDashes(const std::vector<double> dashes)
  {
    dash=dashes;

    return *this;
  }

  LineStyle& LineStyle::SetPriority(int priority)
  {
    this->priority=priority;

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
      case attrGapColor:
        gapColor=other.gapColor;
        break;
      case attrDisplayWidth:
        displayWidth=other.displayWidth;
        break;
      case attrWidth:
        width=other.width;
        break;
      case attrDisplayOffset:
        displayOffset=other.displayOffset;
        break;
      case attrOffset:
        offset=other.offset;
        break;
      case attrJoinCap:
        joinCap=other.joinCap;
        break;
      case attrEndCap:
        endCap=other.endCap;
        break;
      case attrDashes:
        dash=other.dash;
        break;
      case attrPriority:
        priority=other.priority;
        break;
      }
    }
  }

  bool LineStyle::operator==(const LineStyle& other) const
  {
    if (slot!=other.slot) {
      return false;
    }

    if (lineColor!=other.lineColor) {
      return false;
    }

    if (gapColor!=other.gapColor) {
      return false;
    }

    if (displayWidth!=other.displayWidth) {
      return false;
    }

    if (width!=other.width) {
      return false;
    }

    if (displayOffset!=other.displayOffset) {
      return false;
    }

    if (offset!=other.offset) {
      return false;
    }

    if (joinCap!=other.joinCap) {
      return false;
    }

    if (endCap!=other.endCap) {
      return false;
    }

    if (dash!=other.dash) {
      return false;
    }

    return priority==other.priority;
  }

  bool LineStyle::operator!=(const LineStyle& other) const
  {
    return !operator==(other);
  }

  bool LineStyle::operator<(const LineStyle& other) const
  {
    if (slot!=other.slot) {
      return slot<other.slot;
    }

    if (lineColor!=other.lineColor) {
      return lineColor<other.lineColor;
    }

    if (gapColor!=other.gapColor) {
      return gapColor<other.gapColor;
    }

    if (displayWidth!=other.displayWidth) {
      return displayWidth<other.displayWidth;
    }

    if (width!=other.width) {
      return width<other.width;
    }

    if (displayOffset!=other.displayOffset) {
      return displayOffset<other.displayOffset;
    }

    if (offset!=other.offset) {
      return offset<other.offset;
    }

    if (joinCap!=other.joinCap) {
      return joinCap<other.joinCap;
    }

    if (endCap!=other.endCap) {
      return endCap<other.endCap;
    }

    if (dash!=other.dash) {
      return dash<other.dash;
    }

    return priority<other.priority;
  }

  FillStyle::FillStyle()
   : fillColor(1.0,0.0,0.0,0.0),
     patternId(0),
     patternMinMag(Magnification::magWorld),
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

  FillStyle& FillStyle::SetPatternMinMag(const Magnification& mag)
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
        patternId=other.patternId;
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

  bool FillStyle::operator==(const FillStyle& other) const
  {
    if (fillColor!=other.fillColor) {
      return false;
    }

    if (pattern!=other.pattern) {
      return false;
    }

    if (patternMinMag!=other.patternMinMag) {
      return false;
    }

    if (borderColor!=other.borderColor) {
      return false;
    }

    if (borderWidth!=other.borderWidth) {
      return false;
    }

    return borderDash==other.borderDash;
  }

  bool FillStyle::operator!=(const FillStyle& other) const
  {
    return !operator==(other);
  }

  bool FillStyle::operator<(const FillStyle& other) const
  {
    if (fillColor!=other.fillColor) {
      return fillColor<other.fillColor;
    }

    if (pattern!=other.pattern) {
      return pattern<other.pattern;
    }

    if (patternMinMag!=other.patternMinMag) {
      return patternMinMag<other.patternMinMag;
    }

    if (borderColor!=other.borderColor) {
      return borderColor<other.borderColor;
    }

    if (borderWidth!=other.borderWidth) {
      return borderWidth<other.borderWidth;
    }

    return borderDash<other.borderDash;
  }

  LabelStyle::LabelStyle()
   : priority(std::numeric_limits<size_t>::max()),
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

  LabelStyle& LabelStyle::SetPriority(size_t priority)
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
     scaleAndFadeMag(1000000),
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

  TextStyle& TextStyle::SetScaleAndFadeMag(const Magnification& mag)
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

  PathShieldStyle::PathShieldStyle()
   : shieldStyle(new ShieldStyle()),
     shieldSpace(3.0)
  {
    // no code
  }

  PathShieldStyle::PathShieldStyle(const PathShieldStyle& style)
   : shieldStyle(new ShieldStyle(*style.GetShieldStyle().Get())),
     shieldSpace(style.shieldSpace)
  {
    // no code
  }

  PathShieldStyle& PathShieldStyle::SetLabel(ShieldStyle::Label label)
  {
    shieldStyle->SetLabel(label);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetPriority(uint8_t priority)
  {
    shieldStyle->SetPriority(priority);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetSize(double size)
  {
    shieldStyle->SetSize(size);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetTextColor(const Color& color)
  {
    shieldStyle->SetTextColor(color);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetBgColor(const Color& color)
  {
    shieldStyle->SetBgColor(color);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetBorderColor(const Color& color)
  {
    shieldStyle->SetBorderColor(color);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetShieldSpace(double shieldSpace)
  {
    this->shieldSpace=shieldSpace;

    return *this;
  }

  void PathShieldStyle::CopyAttributes(const PathShieldStyle& other,
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
        SetLabel(other.GetLabel());
        break;
      case attrTextColor:
        SetTextColor(other.GetTextColor());
        break;
      case attrBgColor:
        SetBgColor(other.GetBgColor());
        break;
      case attrBorderColor:
        SetBorderColor(other.GetBorderColor());
        break;
      case attrShieldSpace:
        this->shieldSpace=other.shieldSpace;
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
    maxX=-std::numeric_limits<double>::max();
    maxY=-std::numeric_limits<double>::max();

    for (std::list<Coord>::const_iterator coord=coords.begin();
         coord!=coords.end();
         ++coord) {
      minX=std::min(minX,coord->x);
      minY=std::min(minY,coord->y);

      maxX=std::max(maxX,coord->x);
      maxY=std::max(maxY,coord->y);
    }
  }

  void PolygonPrimitive::AddCoord(const Coord& coord)
  {
    coords.push_back(coord);
  }

  RectanglePrimitive::RectanglePrimitive(const Coord& topLeft,
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
    minY=topLeft.y;

    maxX=topLeft.x+width;
    maxY=topLeft.y+height;
  }

  CirclePrimitive::CirclePrimitive(const Coord& center,
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
    maxX(-std::numeric_limits<double>::max()),
    maxY(-std::numeric_limits<double>::max())
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
   : iconId(0)
  {
    // no code
  }

  IconStyle::IconStyle(const IconStyle& style)
  {
    this->iconName=style.iconName;
    this->iconId=style.iconId;
  }

  IconStyle& IconStyle::SetSymbol(const SymbolRef& symbol)
  {
    this->symbol=symbol;

    return *this;
  }

  IconStyle& IconStyle::SetIconName(const std::string& iconName)
  {
    this->iconName=iconName;

    return *this;
  }

  IconStyle& IconStyle::SetIconId(size_t id)
  {
    this->iconId=id;

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
        iconId=other.iconId;
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
    bridge(false),
    tunnel(false),
    oneway(false)
  {
    // no code
  }

  StyleFilter::StyleFilter(const StyleFilter& other)
  {
    this->types=other.types;
    this->minLevel=other.minLevel;
    this->maxLevel=other.maxLevel;
    this->bridge=other.bridge;
    this->tunnel=other.tunnel;
    this->oneway=other.oneway;
    this->sizeCondition=other.sizeCondition;
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

  StyleFilter& StyleFilter::SetBridge(bool bridge)
  {
    this->bridge=bridge;

    return *this;
  }

  StyleFilter& StyleFilter::SetTunnel(bool tunnel)
  {
    this->tunnel=tunnel;

    return *this;
  }

  StyleFilter& StyleFilter::SetOneway(bool oneway)
  {
    this->oneway=oneway;

    return *this;
  }

  StyleFilter& StyleFilter::SetSizeCondition(SizeCondition* condition)
  {
    this->sizeCondition=condition;

    return *this;
  }

  StyleCriteria::StyleCriteria()
  : bridge(false),
    tunnel(false),
    oneway(false)
  {
    // no code
  }

  StyleCriteria::StyleCriteria(const StyleFilter& other)
  {
    this->bridge=other.GetBridge();
    this->tunnel=other.GetTunnel();
    this->oneway=other.GetOneway();
    this->sizeCondition=other.GetSizeCondition();
  }

  StyleCriteria::StyleCriteria(const StyleCriteria& other)
  {
    this->bridge=other.bridge;
    this->tunnel=other.tunnel;
    this->oneway=other.oneway;
    this->sizeCondition=other.sizeCondition;
  }

  bool StyleCriteria::operator==(const StyleCriteria& other) const
  {
    return bridge==other.bridge &&
           tunnel==other.tunnel &&
           oneway==other.oneway &&
           sizeCondition==other.sizeCondition;
  }

  bool StyleCriteria::operator!=(const StyleCriteria& other) const
  {
    return bridge!=other.bridge ||
           tunnel!=other.tunnel ||
           oneway!=other.oneway ||
           sizeCondition!=other.sizeCondition;
  }

  bool StyleCriteria::Matches(double meterInPixel,
                              double meterInMM) const
  {
    if (bridge) {
      return false;
    }

    if (tunnel) {
      return false;
    }

    if (oneway) {
      return false;
    }

    if (sizeCondition.Valid()) {
      if (!sizeCondition->Evaluate(meterInPixel,meterInMM)) {
        return false;
      }
    }

    return true;
  }

  bool StyleCriteria::Matches(const AreaAttributes& /*attributes*/,
                              double meterInPixel,
                              double meterInMM) const
  {
    if (sizeCondition.Valid()) {
      if (!sizeCondition->Evaluate(meterInPixel,meterInMM)) {
        return false;
      }
    }

    return true;
  }

  bool StyleCriteria::Matches(const WayAttributes& attributes,
                              double meterInPixel,
                              double meterInMM) const
  {
    if (bridge &&
        !attributes.IsBridge()) {
      return false;
    }

    if (tunnel &&
        !attributes.IsTunnel()) {
      return false;
    }

    if (oneway &&
        !attributes.GetAccess().IsOneway()) {
      return false;
    }

    if (sizeCondition.Valid()) {
      if (!sizeCondition->Evaluate(meterInPixel,meterInMM)) {
        return false;
      }
    }

    return true;
  }

  StyleConfig::StyleConfig(const TypeConfigRef& typeConfig)
   : typeConfig(typeConfig)
  {
    wayPrio.resize(typeConfig->GetMaxTypeId()+1,std::numeric_limits<size_t>::max());
  }

  StyleConfig::~StyleConfig()
  {
    // no code
  }

  StyleVariableRef StyleConfig::GetVariableByName(const std::string& name) const
  {
    StyleVariableRef result;

    OSMSCOUT_HASHMAP<std::string,StyleVariableRef>::const_iterator entry=variables.find(name);

    if (entry!=variables.end()) {
      result=entry->second;
    }

    return result;
  }

  void StyleConfig::AddVariable(const std::string& name,
                                const StyleVariableRef& variable)
  {
    variables.insert(std::make_pair(name,variable));
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
      if (typeConfig->GetTypeInfo(t)->CanBeNode()) {
        types.push_back(t);
      }
    }
  }

  void StyleConfig::GetAllWayTypes(std::list<TypeId>& types)
  {
    for (TypeId t=0; t<=typeConfig->GetMaxTypeId(); t++) {
      if (typeConfig->GetTypeInfo(t)->CanBeWay()) {
        types.push_back(t);
      }
    }
  }

  void StyleConfig::GetAllAreaTypes(std::list<TypeId>& types)
  {
    for (TypeId t=0; t<=typeConfig->GetMaxTypeId(); t++) {
      if (typeConfig->GetTypeInfo(t)->CanBeArea()) {
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

    for (TypeId type=0;
        type<selectors.size();
        type++) {
      for (size_t level=0; level<selectors[type].size(); level++) {

        if (selectors[type][level].size()>=2) {
          // If two consecutive conditions are equal, one can be removed and the style can get merged
          typename std::list<StyleSelector<S,A> >::iterator prevSelector=selectors[type][level].begin();
          typename std::list<StyleSelector<S,A> >::iterator curSelector=prevSelector;

          curSelector++;

          while (curSelector!=selectors[type][level].end()) {
            if (prevSelector->criteria==curSelector->criteria) {
              prevSelector->attributes.insert(curSelector->attributes.begin(),
                                              curSelector->attributes.end());
              prevSelector->style=new S(*prevSelector->style);
              prevSelector->style->CopyAttributes(curSelector->style,
                                                  curSelector->attributes);

              curSelector=selectors[type][level].erase(curSelector);
            }
            else {
              prevSelector=curSelector;
              curSelector++;
            }
          }
        }

        // If there is only one conditional and it is not visible, we can remove it
        if (selectors[type][level].size()==1 &&
            !selectors[type][level].front().style->IsVisible()) {
          selectors[type][level].clear();
        }
      }
    }

    for (TypeId type=0;
        type<selectors.size();
        type++) {
      for (size_t level=0; level<selectors[type].size(); level++) {
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
    GetMaxLevelInConditionals(wayPathShieldStyleConditionals,
                              maxLevel);

    OSMSCOUT_HASHMAP<std::string,std::list<LineConditionalStyle> > lineStyleBySlot;

    for (std::list<LineConditionalStyle>::const_iterator entry=wayLineStyleConditionals.begin();
         entry!=wayLineStyleConditionals.end();
         ++entry) {
      lineStyleBySlot[entry->style.style->GetSlot()].push_back(*entry);
    }

    wayLineStyleSelectors.resize(lineStyleBySlot.size());

    size_t idx=0;
    for (OSMSCOUT_HASHMAP<std::string,std::list<LineConditionalStyle> >::iterator entry=lineStyleBySlot.begin();
         entry!=lineStyleBySlot.end();
         ++entry) {
      SortInConditionals(*typeConfig,
                         entry->second,
                         maxLevel,
                         wayLineStyleSelectors[idx]);

      idx++;
    }

    SortInConditionals(*typeConfig,
                       wayPathTextStyleConditionals,
                       maxLevel,
                       wayPathTextStyleSelectors);
    SortInConditionals(*typeConfig,
                       wayPathSymbolStyleConditionals,
                       maxLevel,
                       wayPathSymbolStyleSelectors);
    SortInConditionals(*typeConfig,
                       wayPathShieldStyleConditionals,
                       maxLevel,
                       wayPathShieldStyleSelectors);

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
          if (!typeConfig->GetTypeInfo(type)->CanBeWay() ||
              wayPrio[type]!=*prio) {
            continue;
          }

          for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
            if (!wayLineStyleSelectors[slot][type][level].empty()) {
              typeSet.SetType(type);
            }
          }

          if (!wayPathTextStyleSelectors[type][level].empty()) {
            typeSet.SetType(type);
          }
          else if (!wayPathSymbolStyleSelectors[type][level].empty()) {
            typeSet.SetType(type);
          }
          else if (!wayPathShieldStyleSelectors[type][level].empty()) {
            typeSet.SetType(type);
          }
        }

        if (typeSet.HasTypes()) {
          wayTypeSets[level].push_back(typeSet);
        }
      }
    }

    wayLineStyleConditionals.clear();
    wayPathTextStyleConditionals.clear();
    wayPathSymbolStyleConditionals.clear();
    wayPathShieldStyleConditionals.clear();
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

  void StyleConfig::PostprocessIconId()
  {
    OSMSCOUT_HASHMAP<std::string,size_t> symbolIdMap;
    size_t                               nextId=1;

    for (size_t type=0; type<areaIconStyleSelectors.size(); type++) {
      for (size_t level=0; level<areaIconStyleSelectors[type].size(); level++) {
        for (std::list<IconStyleSelector>::iterator selector=areaIconStyleSelectors[type][level].begin();
             selector!=areaIconStyleSelectors[type][level].end();
             ++selector) {
          if (!selector->style->GetIconName().empty()) {
            OSMSCOUT_HASHMAP<std::string,size_t>::iterator entry=symbolIdMap.find(selector->style->GetIconName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector->style->GetIconName(),nextId));

              selector->style->SetIconId(nextId);

              nextId++;
            }
            else {
              selector->style->SetIconId(entry->second);
            }
          }
        }
      }
    }

    for (size_t type=0; type<nodeIconStyleSelectors.size(); type++) {
      for (size_t level=0; level<nodeIconStyleSelectors[type].size(); level++) {
        for (std::list<IconStyleSelector>::iterator selector=nodeIconStyleSelectors[type][level].begin();
             selector!=nodeIconStyleSelectors[type][level].end();
             ++selector) {
          if (!selector->style->GetIconName().empty()) {
            OSMSCOUT_HASHMAP<std::string,size_t>::iterator entry=symbolIdMap.find(selector->style->GetIconName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector->style->GetIconName(),nextId));

              selector->style->SetIconId(nextId);

              nextId++;
            }
            else {
              selector->style->SetIconId(entry->second);
            }
          }
        }
      }
    }
  }

  void StyleConfig::PostprocessPatternId()
  {
    OSMSCOUT_HASHMAP<std::string,size_t> symbolIdMap;
    size_t                               nextId=1;

    for (size_t type=0; type<areaFillStyleSelectors.size(); type++) {
      for (size_t level=0; level<areaFillStyleSelectors[type].size(); level++) {
        for (std::list<FillStyleSelector>::iterator selector=areaFillStyleSelectors[type][level].begin();
             selector!=areaFillStyleSelectors[type][level].end();
             ++selector) {
          if (!selector->style->GetPatternName().empty()) {
            OSMSCOUT_HASHMAP<std::string,size_t>::iterator entry=symbolIdMap.find(selector->style->GetPatternName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector->style->GetPatternName(),nextId));

              selector->style->SetPatternId(nextId);

              nextId++;
            }
            else {
              selector->style->SetPatternId(entry->second);
            }
          }
        }
      }
    }
  }

  void StyleConfig::Postprocess()
  {
    PostprocessNodes();
    PostprocessWays();
    PostprocessAreas();

    PostprocessIconId();
    PostprocessPatternId();
  }

  TypeConfigRef StyleConfig::GetTypeConfig() const
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

  void StyleConfig::AddWayPathShieldStyle(const StyleFilter& filter,
                                         PathShieldPartialStyle& style)
  {
    PathShieldConditionalStyle conditional(filter,style);

    wayPathShieldStyleConditionals.push_back(conditional);
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

  void StyleConfig::GetNodeTypesWithMaxMag(const Magnification& maxMag,
                                           TypeSet& types) const
  {
    if (!nodeTypeSets.empty()) {
      types=nodeTypeSets[std::min((size_t)maxMag.GetLevel(),nodeTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetWayTypesByPrioWithMaxMag(const Magnification& maxMag,
                                                std::vector<TypeSet>& types) const
  {
    if (!wayTypeSets.empty()) {
      types=wayTypeSets[std::min((size_t)maxMag.GetLevel(),wayTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetAreaTypesWithMaxMag(const Magnification& maxMag,
                                           TypeSet& types) const
  {
    if (!areaTypeSets.empty()) {
      types=areaTypeSets[std::min((size_t)maxMag.GetLevel(),areaTypeSets.size()-1)];
    }
  }

  template <class S, class A>
  void GetStyle(const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                const Projection& projection,
                double dpi,
                Ref<S>& style)
  {
    bool   fastpath=style.Invalid();
    bool   composed=false;
    size_t level=projection.GetMagnification().GetLevel();
    double meterInPixel=1/projection.GetPixelSize();
    double meterInMM=meterInPixel*25.4/dpi;

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (typename std::list<StyleSelector<S,A> >::const_iterator s=styleSelectors[level].begin();
         s!=styleSelectors[level].end();
         ++s) {
      const StyleSelector<S,A>& selector=*s;

      if (!selector.criteria.Matches(meterInPixel,
                                     meterInMM)) {
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
      composed=true;
    }

    if (composed &&
        !style->IsVisible()) {
      style=NULL;
    }
  }

  template <class S, class A>
  void GetNodeStyle(const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                    const Node& /*node*/,
                    const Projection& projection,
                    double dpi,
                    Ref<S>& style)
  {
    bool   fastpath=false;
    bool   composed=false;
    size_t level=projection.GetMagnification().GetLevel();
    double meterInPixel=1/projection.GetPixelSize();
    double meterInMM=meterInPixel*25.4/dpi;

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (typename std::list<StyleSelector<S,A> >::const_iterator s=styleSelectors[level].begin();
         s!=styleSelectors[level].end();
         ++s) {
      const StyleSelector<S,A>& selector=*s;

      if (!selector.criteria.Matches(meterInPixel,
                                     meterInMM)) {
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
      composed=true;
    }

    if (composed &&
        !style->IsVisible()) {
      style=NULL;
    }
  }

  /**
   * Get the style data based on the given attributes of an object (OA, either AreaAttributes or WayAttributes),
   * a given style (S) and its style attributes (A).
   */
  template <class S, class A, class OA>
  void GetObjectAttributesStyle(const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                                const OA& attributes,
                                const Projection& projection,
                                double dpi,
                                Ref<S>& style)
  {
    bool   fastpath=false;
    bool   composed=false;
    size_t level=projection.GetMagnification().GetLevel();
    double meterInPixel=1/projection.GetPixelSize();
    double meterInMM=meterInPixel*25.4/dpi;

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (typename std::list<StyleSelector<S,A> >::const_iterator s=styleSelectors[level].begin();
         s!=styleSelectors[level].end();
         ++s) {
      const StyleSelector<S,A>& selector=*s;

      if (!selector.criteria.Matches(attributes,
                                     meterInPixel,
                                     meterInMM)) {
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
      composed=true;
    }

    if (composed &&
        !style->IsVisible()) {
      style=NULL;
    }
  }

  void StyleConfig::GetNodeTextStyle(const Node& node,
                                     const Projection& projection,
                                     double dpi,
                                     TextStyleRef& textStyle) const
  {
    GetNodeStyle(nodeTextStyleSelectors[node.GetType()],
                 node,
                 projection,
                 dpi,
                 textStyle);
  }

  void StyleConfig::GetNodeIconStyle(const Node& node,
                                     const Projection& projection,
                                     double dpi,
                                     IconStyleRef& iconStyle) const
  {
    GetNodeStyle(nodeIconStyleSelectors[node.GetType()],
                 node,
                 projection,
                 dpi,
                 iconStyle);
  }

  void StyleConfig::GetWayLineStyles(const WayAttributes& way,
                                     const Projection& projection,
                                     double dpi,
                                     std::vector<LineStyleRef>& lineStyles) const
  {
    LineStyleRef style;

    lineStyles.clear();
    lineStyles.reserve(wayLineStyleSelectors.size());

    for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
      style=NULL;

      GetObjectAttributesStyle(wayLineStyleSelectors[slot][way.GetType()],
                               way,
                               projection,
                               dpi,
                               style);

      if (style.Valid()) {
        lineStyles.push_back(style);
      }
    }
  }

  void StyleConfig::GetWayPathTextStyle(const WayAttributes& way,
                                        const Projection& projection,
                                        double dpi,
                                        PathTextStyleRef& pathTextStyle) const
  {
    GetObjectAttributesStyle(wayPathTextStyleSelectors[way.GetType()],
                             way,
                             projection,
                             dpi,
                             pathTextStyle);
  }

  void StyleConfig::GetWayPathSymbolStyle(const WayAttributes& way,
                                          const Projection& projection,
                                          double dpi,
                                          PathSymbolStyleRef& pathSymbolStyle) const
  {
    GetObjectAttributesStyle(wayPathSymbolStyleSelectors[way.GetType()],
                             way,
                             projection,
                             dpi,
                             pathSymbolStyle);
  }

  void StyleConfig::GetWayPathShieldStyle(const WayAttributes& way,
                                          const Projection& projection,
                                          double dpi,
                                          PathShieldStyleRef& pathShieldStyle) const
  {
    GetObjectAttributesStyle(wayPathShieldStyleSelectors[way.GetType()],
                             way,
                             projection,
                             dpi,
                             pathShieldStyle);
  }

  void StyleConfig::GetAreaFillStyle(const TypeId& type,
                                     const AreaAttributes& area,
                                     const Projection& projection,
                                     double dpi,
                                     FillStyleRef& fillStyle) const
  {
    GetObjectAttributesStyle(areaFillStyleSelectors[type],
                             area,
                             projection,
                             dpi,
                             fillStyle);
  }

  void StyleConfig::GetAreaTextStyle(const TypeId& type,
                                     const AreaAttributes& area,
                                     const Projection& projection,
                                     double dpi,
                                     TextStyleRef& textStyle) const
  {
    GetObjectAttributesStyle(areaTextStyleSelectors[type],
                             area,
                             projection,
                             dpi,
                             textStyle);
  }

  void StyleConfig::GetAreaIconStyle(const TypeId& type,
                                     const AreaAttributes& area,
                                     const Projection& projection,
                                     double dpi,
                                     IconStyleRef& iconStyle) const
  {
    GetObjectAttributesStyle(areaIconStyleSelectors[type],
                             area,
                             projection,
                             dpi,
                             iconStyle);
  }

  void StyleConfig::GetLandFillStyle(const Projection& projection,
                                     double dpi,
                                     FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileLand],
             projection,
             dpi,
             fillStyle);
  }

  void StyleConfig::GetSeaFillStyle(const Projection& projection,
                                    double dpi,
                                    FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileSea],
             projection,
             dpi,
             fillStyle);
  }

  void StyleConfig::GetCoastFillStyle(const Projection& projection,
                                      double dpi,
                                      FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileCoast],
             projection,
             dpi,
             fillStyle);
  }

  void StyleConfig::GetUnknownFillStyle(const Projection& projection,
                                        double dpi,
                                        FillStyleRef& fillStyle) const
  {
    GetStyle(areaFillStyleSelectors[typeConfig->typeTileUnknown],
             projection,
             dpi,
             fillStyle);
  }

  void StyleConfig::GetCoastlineLineStyle(const Projection& projection,
                                          double dpi,
                                          LineStyleRef& lineStyle) const
  {
    for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
      GetStyle(wayLineStyleSelectors[slot][typeConfig->typeTileCoastline],
               projection,
               dpi,
               lineStyle);
    }
  }

  bool StyleConfig::Load(const std::string& styleFile)
  {
    FileOffset fileSize;
    FILE*      file;
    bool       success=false;

    if (!GetFileSize(styleFile,fileSize)) {
      std::cerr << "Cannot get size of file '" << styleFile << "'" << std::endl;

      return false;
    }

    file=fopen(styleFile.c_str(),"rb");
    if (file==NULL) {
      std::cerr << "Cannot open file '" << styleFile << "'" << std::endl;

      return false;
    }

    unsigned char* content=new unsigned char[fileSize];

    if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
      std::cerr << "Cannot load file '" << styleFile << "'" << std::endl;
      delete [] content;
      fclose(file);

      return NULL;
    }

    fclose(file);

    oss::Scanner *scanner=new oss::Scanner(content,
                                           fileSize);
    oss::Parser  *parser=new oss::Parser(scanner,
                                         *this);

    delete [] content;

    parser->Parse();

    success=!parser->errors->hasErrors;

    delete parser;
    delete scanner;

    Postprocess();

    return success;
  }

}

