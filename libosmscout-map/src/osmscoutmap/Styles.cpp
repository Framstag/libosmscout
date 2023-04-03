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

#include <osmscoutmap/Styles.h>

#include <algorithm>
#include <tuple>

namespace osmscout {

  bool IsLaneOffset(OffsetRel rel)
  {
    return rel==OffsetRel::laneForwardLeft ||
           rel==OffsetRel::laneForwardThroughLeft ||
           rel==OffsetRel::laneForwardThrough ||
           rel==OffsetRel::laneForwardThroughRight ||
           rel==OffsetRel::laneForwardRight ||
           rel==OffsetRel::laneBackwardLeft ||
           rel==OffsetRel::laneBackwardThroughLeft ||
           rel==OffsetRel::laneBackwardThrough ||
           rel==OffsetRel::laneBackwardThroughRight ||
           rel==OffsetRel::laneBackwardRight;
  }

  OffsetRel ParseForwardTurnStringToOffset(LaneTurn turn)
  {
    if (turn==LaneTurn::Left || turn==LaneTurn::MergeToLeft || turn==LaneTurn::SlightLeft || turn==LaneTurn::SharpLeft) {
      return OffsetRel::laneForwardLeft;
    }

    if (turn==LaneTurn::Through_Left || turn==LaneTurn::Through_SlightLeft || turn==LaneTurn::Through_SharpLeft) {
      return OffsetRel::laneForwardThroughLeft;
    }

    if (turn==LaneTurn::Through) {
      return OffsetRel::laneForwardThrough;
    }

    if (turn==LaneTurn::Through_Right || turn==LaneTurn::Through_SlightRight || turn==LaneTurn::Through_SharpRight) {
      return OffsetRel::laneForwardThroughRight;
    }

    if (turn==LaneTurn::Right || turn==LaneTurn::MergeToRight || turn==LaneTurn::SlightRight || turn==LaneTurn::SharpRight) {
      return OffsetRel::laneForwardRight;
    }

    return OffsetRel::base;
  }

  OffsetRel ParseBackwardTurnStringToOffset(LaneTurn turn)
  {
    if (turn==LaneTurn::Left || turn==LaneTurn::MergeToLeft || turn==LaneTurn::SlightLeft || turn==LaneTurn::SharpLeft) {
      return OffsetRel::laneBackwardLeft;
    }

    if (turn==LaneTurn::Through_Left || turn==LaneTurn::Through_SlightLeft || turn==LaneTurn::Through_SharpLeft) {
      return OffsetRel::laneBackwardThroughLeft;
    }

    if (turn==LaneTurn::Through) {
      return OffsetRel::laneBackwardThrough;
    }

    if (turn==LaneTurn::Through_Right || turn==LaneTurn::Through_SlightRight || turn==LaneTurn::Through_SharpRight) {
      return OffsetRel::laneBackwardThroughRight;
    }

    if (turn==LaneTurn::Right || turn==LaneTurn::MergeToRight || turn==LaneTurn::SlightRight || turn==LaneTurn::SharpRight) {
      return OffsetRel::laneBackwardRight;
    }

    return OffsetRel::base;
  }


class LineStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    LineStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("color",LineStyle::attrLineColor));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("gapColor",LineStyle::attrGapColor));
      AddAttribute(std::make_shared<StyleBoolAttributeDescriptor>("preferColorFeature",LineStyle::attrPreferColorFeature));
      AddAttribute(std::make_shared<StyleUDisplayAttributeDescriptor>("displayWidth",LineStyle::attrDisplayWidth));
      AddAttribute(std::make_shared<StyleUMapAttributeDescriptor>("width",LineStyle::attrWidth));
      AddAttribute(std::make_shared<StyleDisplayAttributeDescriptor>("displayOffset",LineStyle::attrDisplayOffset));
      AddAttribute(std::make_shared<StyleMapAttributeDescriptor>("offset",LineStyle::attrOffset));
      AddAttribute(std::make_shared<CapStyleEnumAttributeDescriptor>("joinCap",LineStyle::attrJoinCap));
      AddAttribute(std::make_shared<CapStyleEnumAttributeDescriptor>("endCap",LineStyle::attrEndCap));
      AddAttribute(std::make_shared<StyleUDoubleArrayAttributeDescriptor>("dash",LineStyle::attrDashes));
      AddAttribute(std::make_shared<StyleIntAttributeDescriptor>("priority",LineStyle::attrPriority));
      AddAttribute(std::make_shared<StyleIntAttributeDescriptor>("zIndex",LineStyle::attrZIndex));
      AddAttribute(std::make_shared<OffsetRelAttributeDescriptor>("offsetRel",LineStyle::attrOffsetRel));
    }
  };

  static const StyleDescriptorRef lineStyleDescriptor=std::make_shared<LineStyleDescriptor>();

  LineStyle::LineStyle()
   : lineColor(1.0,0.0,0.0,0.0),
     gapColor(1,0.0,0.0,0.0),
     preferColorFeature(false),
     displayWidth(0.0),
     width(0.0),
     displayOffset(0.0),
     offset(0.0),
     joinCap(capRound),
     endCap(capRound),
     priority(0),
     zIndex(0),
     offsetRel(OffsetRel::base)
  {
    // no code
  }

  void LineStyle::SetColorValue(int attribute, const Color& value)
  {
    switch (attribute) {
    case attrLineColor:
      SetLineColor(value);
      break;
    case attrGapColor:
      SetGapColor(value);
      break;
    default:
      assert(false);
    }
  }

  void LineStyle::SetDoubleValue(int attribute, double value)
  {
    switch (attribute) {
    case attrDisplayWidth:
      SetDisplayWidth(value);
      break;
    case attrWidth:
      SetWidth(value);
      break;
    case attrDisplayOffset:
      SetDisplayOffset(value);
      break;
    case attrOffset:
      SetOffset(value);
      break;
    default:
      assert(false);
    }
  }

  void LineStyle::SetDoubleArrayValue(int attribute, const std::vector<double>& value)
  {
    switch (attribute) {
    case attrDashes:
      SetDashes(value);
      break;
    default:
      assert(false);
    }
  }

  void LineStyle::SetIntValue(int attribute, int value)
  {
    switch (attribute) {
    case attrJoinCap:
      SetJoinCap((CapStyle)value);
      break;
    case attrEndCap:
      SetEndCap((CapStyle)value);
      break;
    case attrPriority:
      SetPriority(value);
      break;
    case attrZIndex:
      SetZIndex(value);
      break;
    case attrOffsetRel:
      SetOffsetRel((OffsetRel)value);
      break;
    default:
      assert(false);
    }
  }

  void LineStyle::SetBoolValue(int attribute, bool value)
  {
    switch (attribute) {
    case attrPreferColorFeature:
      preferColorFeature=value;
      break;
    default:
      assert(false);
    }
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

  LineStyle& LineStyle::SetPreferColorFeature(bool value)
  {
    preferColorFeature=value;

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

  LineStyle& LineStyle::SetDashes(const std::vector<double>& dashes)
  {
    dash=dashes;

    return *this;
  }

  LineStyle& LineStyle::SetPriority(int priority)
  {
    this->priority=priority;

    return *this;
  }

  LineStyle& LineStyle::SetZIndex(int zIndex)
  {
    this->zIndex=zIndex;

    return *this;
  }

  LineStyle& LineStyle::SetOffsetRel(OffsetRel offsetRel)
  {
    this->offsetRel=offsetRel;

    return *this;
  }

  StyleDescriptorRef LineStyle::GetDescriptor()
  {
    return lineStyleDescriptor;
  }

  void LineStyle::CopyAttributes(const LineStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrLineColor:
        lineColor=other.lineColor;
        break;
      case attrGapColor:
        gapColor=other.gapColor;
        break;
      case attrPreferColorFeature:
        preferColorFeature=other.preferColorFeature;
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
      case attrZIndex:
        zIndex=other.zIndex;
        break;
      case attrOffsetRel:
        offsetRel=other.offsetRel;
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

    if (priority!=other.priority) {
      return false;
    }

    if (zIndex!=other.zIndex) {
      return false;
    }

    return offsetRel==other.offsetRel;
  }

  bool LineStyle::operator!=(const LineStyle& other) const
  {
    return !operator==(other);
  }

  bool LineStyle::operator<(const LineStyle& other) const
  {
    return std::tie(slot, lineColor, gapColor, displayWidth, width,
                    displayOffset, offset, joinCap, endCap, dash,
                    priority, zIndex, offsetRel)
           < std::tie(other.slot, other.lineColor, other.gapColor, other.displayWidth, other.width,
                      other.displayOffset, other.offset, other.joinCap, other.endCap, other.dash,
                      other.priority, other.zIndex, other.offsetRel);
  }

  class FillStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    FillStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("color",FillStyle::attrFillColor));
      AddAttribute(std::make_shared<StyleStringAttributeDescriptor>("pattern",FillStyle::attrPattern));
      AddAttribute(std::make_shared<StyleMagnificationAttributeDescriptor>("patternMinMag",FillStyle::attrPatternMinMag));
    }
  };

  static const StyleDescriptorRef fillStyleDescriptor=std::make_shared<FillStyleDescriptor>();

  FillStyle::FillStyle()
   : fillColor(1.0,0.0,0.0,0.0),
     patternId(0),
     patternMinMag(Magnification::magWorld)
  {
    // no code
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

  StyleDescriptorRef FillStyle::GetDescriptor()
  {
    return fillStyleDescriptor;
  }

  void FillStyle::CopyAttributes(const FillStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
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
      }
    }
  }

  bool FillStyle::operator==(const FillStyle& other) const
  {
    return std::tie(fillColor, pattern, patternMinMag) == std::tie(other.fillColor, other.pattern, other.patternMinMag);;
  }

  bool FillStyle::operator!=(const FillStyle& other) const
  {
    return !operator==(other);
  }

  bool FillStyle::operator<(const FillStyle& other) const
  {
    return std::tie(fillColor, pattern, patternMinMag) < std::tie(other.fillColor, other.pattern, other.patternMinMag);
  }

  void FillStyle::SetStringValue(int attribute,
                                 const std::string& value)
  {
    switch (attribute) {
    case attrPattern:
      SetPattern(value);
      break;
    default:
      assert(false);
    }
  }

  void FillStyle::SetColorValue(int attribute,
                                const Color& value)
  {
    switch (attribute) {
    case attrFillColor:
      SetFillColor(value);
      break;
    default:
      assert(false);
    }
  }

  void FillStyle::SetMagnificationValue(int attribute,
                                        const Magnification& value)
  {
    switch (attribute) {
    case attrPatternMinMag:
      SetPatternMinMag(value);
      break;
    default:
      assert(false);
    }
  }

  class BorderStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    BorderStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("color",BorderStyle::attrColor));
      AddAttribute(std::make_shared<StyleUDisplayAttributeDescriptor>("width",BorderStyle::attrWidth));
      AddAttribute(std::make_shared<StyleUDoubleArrayAttributeDescriptor>("dash",BorderStyle::attrDashes));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("gapColor",BorderStyle::attrGapColor));
      AddAttribute(std::make_shared<StyleDisplayAttributeDescriptor>("displayOffset",BorderStyle::attrDisplayOffset));
      AddAttribute(std::make_shared<StyleMapAttributeDescriptor>("offset",BorderStyle::attrOffset));
      AddAttribute(std::make_shared<StyleIntAttributeDescriptor>("priority",BorderStyle::attrPriority));
    }
  };

  static const StyleDescriptorRef borderStyleDescriptor=std::make_shared<BorderStyleDescriptor>();

  BorderStyle::BorderStyle()
    : color(1.0,0.0,0.0,0.0),
      width(0.0),
      displayOffset(0.0),
      offset(0.0),
      priority(std::numeric_limits<int>::max())
  {
    // no code
  }

  BorderStyle& BorderStyle::SetSlot(const std::string& slot)
  {
    this->slot=slot;

    return *this;
  }

  void BorderStyle::SetColorValue(int attribute, const Color& value)
  {
    switch (attribute) {
    case attrColor:
      SetColor(value);
      break;
    case attrGapColor:
      SetColor(value);
      break;
    default:
      assert(false);
    }
  }

  void BorderStyle::SetDoubleValue(int attribute, double value)
  {
    switch (attribute) {
    case attrWidth:
      SetWidth(value);
      break;
    case attrDisplayOffset:
      SetDisplayOffset(value);
      break;
    case attrOffset:
      SetOffset(value);
      break;
    default:
      assert(false);
    }
  }

  void BorderStyle::SetDoubleArrayValue(int attribute, const std::vector<double>& value)
  {
    switch (attribute) {
    case attrDashes:
      SetDashes(value);
      break;
    default:
      assert(false);
    }
  }

  void BorderStyle::SetIntValue(int attribute, int value)
  {
    switch (attribute) {
    case attrPriority:
      SetPriority(value);
      break;
    default:
      assert(false);
    }
  }

  BorderStyle& BorderStyle::SetColor(const Color& color)
  {
    this->color=color;

    return *this;
  }

  BorderStyle& BorderStyle::SetGapColor(const Color& gapColor)
  {
    this->gapColor=gapColor;

    return *this;
  }

  BorderStyle& BorderStyle::SetWidth(double value)
  {
    width=value;

    return *this;
  }

  BorderStyle& BorderStyle::SetDashes(const std::vector<double>& dashes)
  {
    dash=dashes;

    return *this;
  }

  BorderStyle& BorderStyle::SetDisplayOffset(double value)
  {
    displayOffset=value;

    return *this;
  }

  BorderStyle& BorderStyle::SetOffset(double value)
  {
    offset=value;

    return *this;
  }

  BorderStyle& BorderStyle::SetPriority(int priority)
  {
    this->priority=priority;

    return *this;
  }

  StyleDescriptorRef BorderStyle::GetDescriptor()
  {
    return borderStyleDescriptor;
  }

  void BorderStyle::CopyAttributes(const BorderStyle& other,
                                   const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrColor:
        color=other.color;
        break;
      case attrGapColor:
        gapColor=other.gapColor;
        break;
      case attrWidth:
        width=other.width;
        break;
      case attrDashes:
        dash=other.dash;
        break;
      case attrDisplayOffset:
        displayOffset=other.displayOffset;
        break;
      case attrOffset:
        offset=other.offset;
        break;
      case attrPriority:
        priority=other.priority;
        break;
      }
    }
  }

  bool BorderStyle::operator==(const BorderStyle& other) const
  {
    if (color!=other.color) {
      return false;
    }

    if (gapColor!=other.gapColor) {
      return false;
    }

    if (width!=other.width) {
      return false;
    }

    if (dash!=other.dash) {
      return false;
    }

    if (displayOffset!=other.displayOffset) {
      return false;
    }

    if (offset!=other.offset) {
      return false;
    }

    return priority==other.priority;
  }

  bool BorderStyle::operator!=(const BorderStyle& other) const
  {
    return !operator==(other);
  }

  bool BorderStyle::operator<(const BorderStyle& other) const
  {
    return std::tie(color, gapColor, width, displayOffset, offset, dash, priority)
           < std::tie(other.color, other.gapColor, other.width, other.displayOffset, other.offset, other.dash, other.priority);
  }

  LabelStyle::LabelStyle()
   : priority(std::numeric_limits<size_t>::max()),
     size(1)
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

  class TextStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    TextStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleLabelAttributeDescriptor>("label",TextStyle::attrLabel));
      AddAttribute(std::make_shared<TextStyleEnumAttributeDescriptor>("style",TextStyle::attrStyle));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("color",TextStyle::attrTextColor));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("emphasizeColor",TextStyle::attrEmphasizeColor));
      AddAttribute(std::make_shared<StyleUDoubleAttributeDescriptor>("size",TextStyle::attrSize));
      AddAttribute(std::make_shared<StyleMagnificationAttributeDescriptor>("scaleMag",TextStyle::attrScaleAndFadeMag));
      AddAttribute(std::make_shared<StyleBoolAttributeDescriptor>("autoSize",TextStyle::attrAutoSize));
      AddAttribute(std::make_shared<StyleUIntAttributeDescriptor>("priority",TextStyle::attrPriority));
      AddAttribute(std::make_shared<StyleUIntAttributeDescriptor>("position",TextStyle::attrPosition));
    }
  };

  static const StyleDescriptorRef textStyleDescriptor=std::make_shared<TextStyleDescriptor>();

  TextStyle::TextStyle()
   : position(0),
     textColor(0,0,0),
     emphasizeColor(1,1,1),
     style(normal),
     scaleAndFadeMag(1000000),
     autoSize(false)
  {
    // no code
  }

  void TextStyle::SetBoolValue(int attribute, bool value)
  {
    switch (attribute) {
    case attrAutoSize:
      SetAutoSize(value);
      break;
    default:
      assert(false);
    }

  }

  void TextStyle::SetColorValue(int attribute, const Color& value)
  {
    switch (attribute) {
    case attrTextColor:
      SetTextColor(value);
      break;
    case attrEmphasizeColor:
      SetEmphasizeColor(value);
      break;
    default:
      assert(false);
    }
  }

  void TextStyle::SetMagnificationValue(int attribute, const Magnification& value)
  {
    switch (attribute) {
    case attrScaleAndFadeMag:
      SetScaleAndFadeMag(value);
      break;
    default:
      assert(false);
    }
  }

  void TextStyle::SetDoubleValue(int attribute, double value)
  {
    switch (attribute) {
    case attrSize:
      SetSize(value);
      break;
    default:
      assert(false);
    }
  }

  void TextStyle::SetIntValue(int attribute, int value)
  {
    switch (attribute) {
    case attrStyle:
      SetStyle((Style)value);
      break;
    default:
      assert(false);
    }
  }

  void TextStyle::SetUIntValue(int attribute, size_t value)
  {
    switch (attribute) {
    case attrPriority:
      SetPriority(value);
      break;
    case attrPosition:
      SetPosition(value);
      break;
    default:
      assert(false);
    }
  }

  void TextStyle::SetLabelValue(int attribute, const LabelProviderRef& value)
  {
    switch (attribute) {
    case attrLabel:
      SetLabel(value);
      break;
    default:
      assert(false);
    }
  }

  TextStyle& TextStyle::SetSlot(const std::string& slot)
  {
    this->slot=slot;

    return *this;
  }

  TextStyle& TextStyle::SetSize(double size)
  {
    LabelStyle::SetSize(size);

    return *this;
  }

  TextStyle& TextStyle::SetLabel(const LabelProviderRef& label)
  {
    this->label=label;

    return *this;
  }

  TextStyle& TextStyle::SetPosition(size_t position)
  {
    this->position=position;

    return *this;
  }

  TextStyle& TextStyle::SetTextColor(const Color& color)
  {
    this->textColor=color;

    return *this;
  }

  TextStyle& TextStyle::SetEmphasizeColor(const Color& color)
  {
    this->emphasizeColor=color;

    return *this;
  }

  TextStyle& TextStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  TextStyle& TextStyle::SetScaleAndFadeMag(const Magnification& mag)
  {
    this->scaleAndFadeMag=mag;

    return *this;
  }

  TextStyle& TextStyle::SetAutoSize(bool autoSize)
  {
    this->autoSize=autoSize;

    return *this;
  }

  StyleDescriptorRef TextStyle::GetDescriptor()
  {
    return textStyleDescriptor;
  }

  void TextStyle::CopyAttributes(const TextStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrPriority:
        SetPriority(other.GetPriority());
        break;
      case attrSize:
        SetSize(other.GetSize());
        break;
      case attrLabel:
        label=other.label;
        break;
      case attrPosition:
        position=other.position;
        break;
      case attrTextColor:
        textColor=other.textColor;
        break;
      case attrEmphasizeColor:
        emphasizeColor=other.emphasizeColor;
        break;
      case attrStyle:
        style=other.style;
        break;
      case attrScaleAndFadeMag:
        scaleAndFadeMag=other.scaleAndFadeMag;
        break;
      case attrAutoSize:
        autoSize=other.autoSize;
        break;
      }
    }
  }

  bool TextStyle::operator==(const TextStyle& other) const
  {
    if (GetPriority()!=other.GetPriority()) {
      return false;
    }

    if (GetSize()!=other.GetSize()) {
      return false;
    }

    return std::tie(slot, label, position, textColor, style,
                    scaleAndFadeMag, autoSize)
           == std::tie(other.slot, other.label, other.position, other.textColor, other.style,
                       other.scaleAndFadeMag, other.autoSize);
  }

  bool TextStyle::operator!=(const TextStyle& other) const
  {
    return !operator==(other);
  }

  bool TextStyle::operator<(const TextStyle& other) const
  {
    if (GetPriority()!=other.GetPriority()) {
      return GetPriority()<other.GetPriority();
    }

    if (GetSize()!=other.GetSize()) {
      return GetSize()<other.GetSize();
    }

    return std::tie(slot, label, position, textColor, style,
                    scaleAndFadeMag, autoSize)
           < std::tie(other.slot, other.label, other.position, other.textColor, other.style,
                      other.scaleAndFadeMag, other.autoSize);
  }

  ShieldStyle::ShieldStyle()
   : textColor(Color::WHITE),
     bgColor(Color::BLACK),
     borderColor(Color::WHITE)
  {
    // no code
  }

  ShieldStyle& ShieldStyle::SetLabel(const LabelProviderRef& label)
  {
    this->label=label;

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
    for (const auto& attribute : attributes) {
      switch (attribute) {
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

  class PathShieldStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    PathShieldStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleLabelAttributeDescriptor>("label",PathShieldStyle::attrLabel));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("color",PathShieldStyle::attrTextColor));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("backgroundColor",PathShieldStyle::attrBgColor));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("borderColor",PathShieldStyle::attrBorderColor));
      AddAttribute(std::make_shared<StyleUDoubleAttributeDescriptor>("size",PathShieldStyle::attrSize));
      AddAttribute(std::make_shared<StyleUIntAttributeDescriptor>("priority",PathShieldStyle::attrPriority));
      AddAttribute(std::make_shared<StyleUDisplayAttributeDescriptor>("shieldSpace",PathShieldStyle::attrShieldSpace));
    }
  };

  static const StyleDescriptorRef pathShieldStyleDescriptor=std::make_shared<PathShieldStyleDescriptor>();

  PathShieldStyle::PathShieldStyle()
   : shieldStyle(std::make_shared<ShieldStyle>()),
     shieldSpace(3.0)
  {
    // no code
  }

  PathShieldStyle::PathShieldStyle(const PathShieldStyle& style)
   : shieldStyle(std::make_shared<ShieldStyle>(*style.GetShieldStyle())),
     shieldSpace(style.shieldSpace)
  {
    // no code
  }

  PathShieldStyle::PathShieldStyle(const PathShieldStyle&& style)
    : shieldStyle(std::make_shared<ShieldStyle>(*style.GetShieldStyle())),
      shieldSpace(style.shieldSpace)
  {
    // no code
  }

  void PathShieldStyle::SetLabelValue(int attribute,
                                      const LabelProviderRef& value)
  {
    switch (attribute) {
    case attrLabel:
      SetLabel(value);
      break;
    default:
      assert(false);
    }
  }


  void PathShieldStyle::SetColorValue(int attribute, const Color& value)
  {
    switch (attribute) {
    case attrTextColor:
      SetTextColor(value);
      break;
    case attrBgColor:
      SetBgColor(value);
      break;
    case attrBorderColor:
      SetBorderColor(value);
      break;
    default:
      assert(false);
    }
  }

  void PathShieldStyle::SetDoubleValue(int attribute, const double value)
  {
    switch (attribute) {
    case attrSize:
      SetSize(value);
      break;
    case attrShieldSpace:
      SetShieldSpace(value);
      break;
    default:
      assert(false);
    }
  }

  void PathShieldStyle::SetUIntValue(int attribute, size_t value)
  {
    switch (attribute) {
    case attrPriority:
      SetPriority(value);
      break;
    default:
      assert(false);
    }
  }

  PathShieldStyle& PathShieldStyle::SetLabel(const LabelProviderRef& label)
  {
    shieldStyle->SetLabel(label);

    return *this;
  }

  PathShieldStyle& PathShieldStyle::SetPriority(size_t priority)
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

  StyleDescriptorRef PathShieldStyle::GetDescriptor()
  {
    return pathShieldStyleDescriptor;
  }

  void PathShieldStyle::CopyAttributes(const PathShieldStyle& other,
                                       const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
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

  class PathTextStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    PathTextStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleLabelAttributeDescriptor>("label",PathTextStyle::attrLabel));
      AddAttribute(std::make_shared<StyleColorAttributeDescriptor>("color",PathTextStyle::attrTextColor));
      AddAttribute(std::make_shared<StyleUDoubleAttributeDescriptor>("size",PathTextStyle::attrSize));
      AddAttribute(std::make_shared<StyleDisplayAttributeDescriptor>("displayOffset",PathTextStyle::attrDisplayOffset));
      AddAttribute(std::make_shared<StyleUMapAttributeDescriptor>("offset",PathTextStyle::attrOffset));
      AddAttribute(std::make_shared<StyleUIntAttributeDescriptor>("priority",PathTextStyle::attrPriority));
    }
  };

  static const StyleDescriptorRef pathTextStyleDescriptor=std::make_shared<PathTextStyleDescriptor>();

  PathTextStyle::PathTextStyle()
   : size(1),
     textColor(0,0,0),
     displayOffset(0.0),
     offset(0.0),
     priority(0)
  {
    // no code
  }

  void PathTextStyle::SetColorValue(int attribute, const Color& value)
  {
    switch (attribute) {
    case attrTextColor:
      SetTextColor(value);
      break;
    default:
      assert(false);
    }
  }

  void PathTextStyle::SetDoubleValue(int attribute, double value)
  {
    switch (attribute) {
    case attrSize:
      SetSize(value);
      break;
    case attrDisplayOffset:
      SetDisplayOffset(value);
      break;
    case attrOffset:
      SetOffset(value);
      break;
    default:
      assert(false);
    }
  }

  void PathTextStyle::SetLabelValue(int attribute, const LabelProviderRef& value)
  {
    switch (attribute) {
    case attrLabel:
      SetLabel(value);
      break;
    default:
      assert(false);
    }
  }

  void PathTextStyle::SetUIntValue(int attribute, size_t value)
  {
    switch (attribute) {
    case attrPriority:
      SetPriority(value);
      break;
    default:
      assert(false);
    }
  }

  PathTextStyle& PathTextStyle::SetLabel(const LabelProviderRef& label)
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

  PathTextStyle& PathTextStyle::SetDisplayOffset(double value)
  {
    this->displayOffset=value;

    return *this;
  }

  PathTextStyle& PathTextStyle::SetOffset(double value)
  {
    this->offset=value;

    return *this;
  }

  PathTextStyle& PathTextStyle::SetPriority(size_t value)
  {
    this->priority=value;

    return *this;
  }

  StyleDescriptorRef PathTextStyle::GetDescriptor()
  {
    return pathTextStyleDescriptor;
  }

  void PathTextStyle::CopyAttributes(const PathTextStyle& other,
                                     const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrLabel:
        label=other.label;
        break;
      case attrSize:
        size=other.size;
        break;
      case attrTextColor:
        textColor=other.textColor;
        break;
      case attrDisplayOffset:
        displayOffset=other.displayOffset;
        break;
      case attrOffset:
        offset=other.offset;
        break;
      case attrPriority:
        priority=other.priority;
        break;
      }
    }
  }

  DrawPrimitive::DrawPrimitive(const FillStyleRef& fillStyle,
                               const BorderStyleRef& borderStyle)
  : fillStyle(fillStyle),
    borderStyle(borderStyle)
  {
    // no code
  }

  PolygonPrimitive::PolygonPrimitive(const FillStyleRef& fillStyle,
                                     const BorderStyleRef& borderStyle)
  : DrawPrimitive(fillStyle,
                  borderStyle)
  {
    // no code
  }

  ScreenBox PolygonPrimitive::GetBoundingBox() const
  {
    double minX=std::numeric_limits<double>::max();
    double minY=std::numeric_limits<double>::max();
    double maxX=-std::numeric_limits<double>::min();
    double maxY=-std::numeric_limits<double>::min();

    for (const auto& coord : coords) {
      minX=std::min(minX,coord.GetX());
      minY=std::min(minY,coord.GetY());

      maxX=std::max(maxX,coord.GetX());
      maxY=std::max(maxY,coord.GetY());
    }

    return {Vertex2D(minX,minY),Vertex2D(maxX,maxY)};
  }

  void PolygonPrimitive::AddCoord(const Vertex2D& coord)
  {
    coords.push_back(coord);
  }

  RectanglePrimitive::RectanglePrimitive(const Vertex2D& topLeft,
                                         double width,
                                         double height,
                                         const FillStyleRef& fillStyle,
                                         const BorderStyleRef& borderStyle)
  : DrawPrimitive(fillStyle,
                  borderStyle),
    topLeft(topLeft),
    width(width),
    height(height)
  {
    // no code
  }

  ScreenBox RectanglePrimitive::GetBoundingBox() const
  {
    return {topLeft,Vertex2D(topLeft.GetX()+width,topLeft.GetY()+height)};
  }

  CirclePrimitive::CirclePrimitive(const Vertex2D& center,
                                   double radius,
                                   const FillStyleRef& fillStyle,
                                   const BorderStyleRef& borderStyle)
  : DrawPrimitive(fillStyle,
                  borderStyle),
    center(center),
    radius(radius)
  {
    // no code
  }

  ScreenBox CirclePrimitive::GetBoundingBox() const
  {
    return {Vertex2D(center.GetX()-radius,
                     center.GetY()-radius),
            Vertex2D(center.GetX()+radius,
                     center.GetY()+radius)};
  }

  Symbol::Symbol(const std::string& name,
                 ProjectionMode projectionMode)
  : name(name),
    projectionMode(projectionMode)
  {
    // no code
  }

  void Symbol::AddPrimitive(const DrawPrimitiveRef& primitive)
  {
    ScreenBox screenBox = primitive->GetBoundingBox();

    switch (projectionMode){
      case ProjectionMode::MAP:
        mapBoundingBox=mapBoundingBox.Merge(screenBox);
        break;
      case ProjectionMode::GROUND:
        groundBoundingBox=groundBoundingBox.Merge(screenBox);
        break;
      default:
        assert(false);
    }

    if (primitive->GetBorderStyle() && primitive->GetBorderStyle()->IsVisible()) {
      maxBorderWidth=std::max(maxBorderWidth, primitive->GetBorderStyle()->GetWidth());
    }

    primitives.push_back(primitive);
  }

  class IconStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    IconStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleSymbolAttributeDescriptor>("symbol",IconStyle::attrSymbol));
      AddAttribute(std::make_shared<StyleStringAttributeDescriptor>("name",IconStyle::attrIconName));
      AddAttribute(std::make_shared<StyleUIntAttributeDescriptor>("position",IconStyle::attrPosition));
      AddAttribute(std::make_shared<StyleUIntAttributeDescriptor>("priority",IconStyle::attrPriority));
      AddAttribute(std::make_shared<StyleBoolAttributeDescriptor>("overlay",IconStyle::attrOverlay));
    }
  };

  static const StyleDescriptorRef iconStyleDescriptor=std::make_shared<IconStyleDescriptor>();

  IconStyle::IconStyle()
   : iconId(0),
     width(14),
     height(14),
     position(0),
     priority(std::numeric_limits<size_t>::max()),
     overlay(false)
  {
    // no code
  }

  void IconStyle::SetBoolValue(int attribute, bool value)
  {
    switch (attribute) {
    case attrOverlay:
      SetOverlay(value);
      break;
    default:
      assert(false);
    }
  }

  void IconStyle::SetStringValue(int attribute,
                                 const std::string& value)
  {
    switch (attribute) {
    case attrIconName:
      SetIconName(value);
      break;
    default:
      assert(false);
    }
  }

  void IconStyle::SetSymbolValue(int attribute,
                                 const SymbolRef& value)
  {
    switch (attribute) {
    case attrSymbol:
      SetSymbol(value);
      break;
    default:
      assert(false);
    }
  }

  void IconStyle::SetUIntValue(int attribute,
                               size_t value)
  {
    switch (attribute) {
    case attrPosition:
      SetPosition(value);
      break;
    case attrPriority:
      SetPriority(value);
      break;
    default:
      assert(false);
    }
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

  IconStyle& IconStyle::SetWidth(unsigned int w)
  {
    this->width=w;

    return *this;
  }

  IconStyle& IconStyle::SetHeight(unsigned int h)
  {
    this->height=h;

    return *this;
  }

  IconStyle& IconStyle::SetPosition(size_t position)
  {
    this->position=position;

    return *this;
  }

  IconStyle& IconStyle::SetPriority(size_t priority)
  {
    this->priority=priority;

    return *this;
  }

  IconStyle& IconStyle::SetOverlay(bool overlay)
  {
    this->overlay=overlay;

    return *this;
  }

  StyleDescriptorRef IconStyle::GetDescriptor()
  {
    return iconStyleDescriptor;
  }

  void IconStyle::CopyAttributes(const IconStyle& other,
                                 const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrSymbol:
        symbol=other.symbol;
        break;
      case attrIconName:
        iconName=other.iconName;
        iconId=other.iconId;
        break;
      case attrPosition:
        position=other.position;
        break;
      case attrPriority:
        priority=other.priority;
        break;
      case attrOverlay:
        overlay=other.overlay;
        break;
      }
    }
  }

  class PathSymbolStyleDescriptor CLASS_FINAL : public StyleDescriptor
  {
  public:
    PathSymbolStyleDescriptor()
    {
      AddAttribute(std::make_shared<StyleSymbolAttributeDescriptor>("symbol",PathSymbolStyle::attrSymbol));
      AddAttribute(std::make_shared<RenderModeEnumAttributeDescriptor>("renderMode",PathSymbolStyle::attrRenderMode));
      AddAttribute(std::make_shared<StyleUDoubleAttributeDescriptor>("scale",PathSymbolStyle::attrScale));
      AddAttribute(std::make_shared<StyleUDisplayAttributeDescriptor>("symbolSpace",PathSymbolStyle::attrSymbolSpace));
      AddAttribute(std::make_shared<StyleDisplayAttributeDescriptor>("displayOffset",PathSymbolStyle::attrDisplayOffset));
      AddAttribute(std::make_shared<StyleUMapAttributeDescriptor>("offset",PathSymbolStyle::attrOffset));
      AddAttribute(std::make_shared<OffsetRelAttributeDescriptor>("offsetRel",PathSymbolStyle::attrOffsetRel));    }
  };

  static const StyleDescriptorRef pathSymbolStyleDescriptor=std::make_shared<PathSymbolStyleDescriptor>();

  PathSymbolStyle& PathSymbolStyle::SetSlot(const std::string& slot)
  {
    this->slot=slot;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetSymbol(const SymbolRef& symbol)
  {
    this->symbol=symbol;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetRenderMode(RenderMode renderMode)
  {
    this->renderMode=renderMode;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetScale(double scale)
  {
    this->scale=scale;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetSymbolSpace(double space)
  {
    symbolSpace=space;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetDisplayOffset(double value)
  {
    this->displayOffset=value;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetOffset(double value)
  {
    this->offset=value;

    return *this;
  }

  PathSymbolStyle& PathSymbolStyle::SetOffsetRel(OffsetRel offsetRel)
  {
    this->offsetRel=offsetRel;

    return *this;
  }

  StyleDescriptorRef PathSymbolStyle::GetDescriptor()
  {
    return pathSymbolStyleDescriptor;
  }

  void PathSymbolStyle::CopyAttributes(const PathSymbolStyle& other,
                                       const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrSymbol:
        symbol=other.symbol;
        break;
      case attrRenderMode:
        renderMode=other.renderMode;
        break;
      case attrScale:
        scale=other.scale;
        break;
      case attrSymbolSpace:
        symbolSpace=other.symbolSpace;
        break;
      case attrDisplayOffset:
        displayOffset=other.displayOffset;
        break;
      case attrOffset:
        offset=other.offset;
        break;
      case attrOffsetRel:
        offsetRel=other.offsetRel;
        break;
      }
    }
  }

  void PathSymbolStyle::SetDoubleValue(int attribute,
                                       double value)
  {
    switch (attribute) {
    case attrScale:
      SetScale(value);
      break;
    case attrOffset:
      SetOffset(value);
      break;
    case attrDisplayOffset:
      SetDisplayOffset(value);
      break;
    case attrSymbolSpace:
      SetSymbolSpace(value);
      break;
    default:
      assert(false);
    }
  }

  void PathSymbolStyle::SetSymbolValue(int attribute,
                                       const SymbolRef& value)
  {
    switch (attribute) {
    case attrSymbol:
      SetSymbol(value);
      break;
    default:
      assert(false);
    }
  }

  void PathSymbolStyle::SetIntValue(int attribute,
                                    int value)
  {
    switch (attribute) {
    case attrRenderMode:
      SetRenderMode((PathSymbolStyle::RenderMode)value);
      break;
    case attrOffsetRel:
      SetOffsetRel((OffsetRel)value);
      break;
    default:
      assert(false);
    }
  }
}
