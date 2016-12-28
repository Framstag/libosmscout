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
#include <osmscout/util/Logger.h>

#include <osmscout/oss/Parser.h>
#include <osmscout/oss/Scanner.h>

namespace osmscout {

  LabelProvider::~LabelProvider()
  {
    // No code
  }

  LabelProviderFactory::~LabelProviderFactory()
  {
    // no code
  }

  DynamicFeatureLabelReader::DynamicFeatureLabelReader(const TypeConfig& typeConfig,
                                                       const std::string& featureName,
                                                       const std::string& labelName)
  {
    FeatureRef feature=typeConfig.GetFeature(featureName);
    size_t     labelIndex;

    assert(feature);
    assert(feature->HasLabel());

    feature->GetLabelIndex(labelName,
                           labelIndex);

    this->featureName=featureName;
    this->labelName=labelName;
    this->labelIndex=labelIndex;

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(featureName,
                          index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  INameLabelProviderFactory::INameLabelProvider::INameLabelProvider(const TypeConfig& typeConfig)
  {
    nameLookupTable.resize(typeConfig.GetTypeCount(),
                           std::numeric_limits<size_t>::max());
    nameAltLookupTable.resize(typeConfig.GetTypeCount(),
                              std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(NameFeature::NAME,
                          index)) {
        nameLookupTable[type->GetIndex()]=index;
      }

      if (type->GetFeature(NameAltFeature::NAME,
                          index)) {
        nameAltLookupTable[type->GetIndex()]=index;
      }
    }
  }

  std::string INameLabelProviderFactory::INameLabelProvider::GetLabel(const MapParameter& parameter,
                                                                      const FeatureValueBuffer& buffer) const
  {
    if (parameter.GetShowAltLanguage()) {
      size_t index=nameAltLookupTable[buffer.GetType()->GetIndex()];

      if (index!=std::numeric_limits<size_t>::max() &&
          buffer.HasFeature(index)) {
        FeatureValue *value=buffer.GetValue(index);

        if (value!=NULL) {
          return value->GetLabel();
        }
      }

      index=nameLookupTable[buffer.GetType()->GetIndex()];

      if (index!=std::numeric_limits<size_t>::max() &&
          buffer.HasFeature(index)) {
        FeatureValue *value=buffer.GetValue(index);

        if (value!=NULL) {
          return value->GetLabel();
        }
      }

      return "";
    }
    else {
      size_t index=nameLookupTable[buffer.GetType()->GetIndex()];

      if (index!=std::numeric_limits<size_t>::max() &&
          buffer.HasFeature(index)) {
        FeatureValue *value=buffer.GetValue(index);

        if (value!=NULL) {
          return value->GetLabel();
        }
      }

      return "";
    }

  }

  LabelProviderRef INameLabelProviderFactory::Create(const TypeConfig& typeConfig) const
  {
    if (!instance) {
      instance=std::make_shared<INameLabelProvider>(typeConfig);
    }

    return instance;
  }

  std::string DynamicFeatureLabelReader::GetLabel(const MapParameter& /*parameter*/,
                                                  const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasFeature(index)) {
      FeatureValue *value=buffer.GetValue(index);

      if (value!=NULL) {
        return value->GetLabel();
      }
    }

    return "";
  }

  StyleResolveContext::StyleResolveContext(const TypeConfigRef& typeConfig)
  : bridgeReader(*typeConfig),
    tunnelReader(*typeConfig),
    accessReader(*typeConfig)
  {
    // no code
  }

  bool StyleResolveContext::IsOneway(const FeatureValueBuffer& buffer) const
  {
    AccessFeatureValue *accessValue=accessReader.GetValue(buffer);

    if (accessValue!=NULL) {
      return accessValue->IsOneway();
    }
    else {
      AccessFeatureValue accessValueDefault(buffer.GetType()->GetDefaultAccess());

      return accessValueDefault.IsOneway();
    }
  }

  StyleConstant::StyleConstant()
  {
    // no code
  }

  StyleConstant::~StyleConstant()
  {
    // no code
  }

  StyleConstantColor::StyleConstantColor(const Color& color)
  : color(color)
  {
    // no code
  }

  StyleConstantMag::StyleConstantMag(Magnification& magnification)
  : magnification(magnification)
  {
    // no code
  }

  StyleConstantUInt::StyleConstantUInt(size_t& value)
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

    return matchesMaxMM || matchesMaxPx;
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
     priority(0),
     zIndex(0)
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
    priority(style.priority),
    zIndex(style.zIndex)
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

  LineStyle& LineStyle::SetZIndex(int zIndex)
  {
    this->zIndex=zIndex;

    return *this;
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

    return zIndex==other.zIndex;
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

    if (priority!=other.priority) {
      return priority<other.priority;
    }

    return zIndex<other.zIndex;
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
   : position(0),
     textColor(0,0,0),
     style(normal),
     scaleAndFadeMag(1000000),
     autoSize(false)
  {
    // no code
  }

  TextStyle::TextStyle(const TextStyle& style)
  : LabelStyle(style),
    slot(style.slot),
    label(style.label),
    position(style.position),
    textColor(style.textColor),
    style(style.style),
    scaleAndFadeMag(style.scaleAndFadeMag),
    autoSize(style.autoSize)
  {
    // no code
  }

  TextStyle& TextStyle::SetSlot(const std::string& slot)
  {
    this->slot=slot;

    return *this;
  }

  TextStyle& TextStyle::SetPriority(uint8_t priority)
  {
    LabelStyle::SetPriority(priority);

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

    if (slot!=other.slot) {
      return false;
    }

    if (label!=other.label) {
      return false;
    }

    if (position!=other.position) {
      return false;
    }

    if (textColor!=other.textColor) {
      return false;
    }

    if (style!=other.style) {
      return false;
    }

    if (scaleAndFadeMag!=other.scaleAndFadeMag) {
      return false;
    }

    if (autoSize!=other.autoSize) {
      return false;
    }

    return true;
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

    if (slot!=other.slot) {
      return slot<other.slot;
    }

    if (label!=other.label) {
      return label<other.label;
    }

    if (position!=other.position) {
      return position<other.position;
    }

    if (textColor!=other.textColor) {
      return textColor<other.textColor;
    }

    if (style!=other.style) {
      return style<other.style;
    }

    if (scaleAndFadeMag!=other.scaleAndFadeMag) {
      return scaleAndFadeMag<other.scaleAndFadeMag;
    }

    return autoSize<other.autoSize;
  }

  ShieldStyle::ShieldStyle()
   : textColor(0,0,0),
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

  ShieldStyle& ShieldStyle::SetLabel(const LabelProviderRef& label)
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

  PathShieldStyle& PathShieldStyle::SetLabel(const LabelProviderRef& label)
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

  PathTextStyle::PathTextStyle()
   : size(1),
     textColor(0,0,0),
     displayOffset(0.0),
     offset(0.0)
  {
    // no code
  }

  PathTextStyle::PathTextStyle(const PathTextStyle& style)
  {
    this->label=style.label;
    this->size=style.size;
    this->textColor=style.textColor;
    this->displayOffset=style.displayOffset;
    this->offset=style.offset;
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

    for (const auto& coord : coords) {
      minX=std::min(minX,coord.GetX());
      minY=std::min(minY,coord.GetY());

      maxX=std::max(maxX,coord.GetX());
      maxY=std::max(maxY,coord.GetY());
    }
  }

  void PolygonPrimitive::AddCoord(const Vertex2D& coord)
  {
    coords.push_back(coord);
  }

  RectanglePrimitive::RectanglePrimitive(const Vertex2D& topLeft,
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
    minX=topLeft.GetX();
    minY=topLeft.GetY()-height;

    maxX=topLeft.GetX()+width;
    maxY=topLeft.GetY();
  }

  CirclePrimitive::CirclePrimitive(const Vertex2D& center,
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
    minX=center.GetX()-radius;
    minY=center.GetY()-radius;

    maxX=center.GetX()+radius;
    maxY=center.GetY()+radius;
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
   : iconId(0),
     position(0)
  {
    // no code
  }

  IconStyle::IconStyle(const IconStyle& style)
  : iconName(style.iconName),
    iconId(style.iconId),
    position(style.position)
  {
    // no code
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

  IconStyle& IconStyle::SetPosition(size_t position)
  {
    this->position=position;

    return *this;
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
      }
    }
  }

  PathSymbolStyle::PathSymbolStyle()
  : symbolSpace(15),
    displayOffset(0.0),
    offset(0.0)
  {
    // no code
  }

  PathSymbolStyle::PathSymbolStyle(const PathSymbolStyle& style)
  : symbol(style.symbol),
    symbolSpace(style.symbolSpace),
    displayOffset(style.displayOffset),
    offset(style.offset)
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


  void PathSymbolStyle::CopyAttributes(const PathSymbolStyle& other,
                                       const std::set<Attribute>& attributes)
  {
    for (const auto& attribute : attributes) {
      switch (attribute) {
      case attrSymbol:
        symbol=other.symbol;
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

  StyleFilter& StyleFilter::SetTypes(const TypeInfoSet& types)
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

  StyleFilter& StyleFilter::SetSizeCondition(const SizeConditionRef& condition)
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

    if (sizeCondition) {
      if (!sizeCondition->Evaluate(meterInPixel,meterInMM)) {
        return false;
      }
    }

    return true;
  }

  bool StyleCriteria::Matches(const StyleResolveContext& context,
                              const FeatureValueBuffer& buffer,
                              double meterInPixel,
                              double meterInMM) const
  {
    if (bridge &&
        !context.IsBridge(buffer)) {
      return false;
    }

    if (tunnel &&
        !context.IsTunnel(buffer)) {
      return false;
    }

    if (oneway &&
        !context.IsOneway(buffer)) {
      return false;
    }

    if (sizeCondition) {
      if (!sizeCondition->Evaluate(meterInPixel,meterInMM)) {
        return false;
      }
    }

    return true;
  }

  StyleConfig::StyleConfig(const TypeConfigRef& typeConfig)
   : typeConfig(typeConfig),
     styleResolveContext(typeConfig)
  {
    log.Debug() << "StyleConfig::StyleConfig()";

    tileLandBuffer.SetType(typeConfig->typeInfoTileLand);
    tileSeaBuffer.SetType(typeConfig->typeInfoTileSea);
    tileCoastBuffer.SetType(typeConfig->typeInfoTileCoast);
    tileUnknownBuffer.SetType(typeConfig->typeInfoTileUnknown);
    coastlineBuffer.SetType(typeConfig->typeInfoCoastline);
    osmTileBorderBuffer.SetType(typeConfig->typeInfoOSMTileBorder);
    osmSubTileBorderBuffer.SetType(typeConfig->typeInfoOSMSubTileBorder);

    LabelProviderFactoryRef labelProviderFactory=std::make_shared<INameLabelProviderFactory>();

    RegisterLabelProviderFactory("IName",labelProviderFactory);
  }

  StyleConfig::~StyleConfig()
  {
    log.Debug() << "StyleConfig::~StyleConfig()";
  }

  void StyleConfig::Reset()
  {
    symbols.clear();
    emptySymbol=NULL;

    nodeTextStyleConditionals.clear();
    nodeIconStyleConditionals.clear();
    nodeTextStyleSelectors.clear();
    nodeIconStyleSelectors.clear();
    nodeTypeSets.clear();

    wayPrio.clear();
    wayLineStyleConditionals.clear();
    wayPathTextStyleConditionals.clear();
    wayPathSymbolStyleConditionals.clear();
    wayPathShieldStyleConditionals.clear();
    wayLineStyleSelectors.clear();
    wayPathTextStyleSelectors.clear();
    wayPathSymbolStyleSelectors.clear();
    wayPathShieldStyleSelectors.clear();
    wayTypeSets.clear();

    areaFillStyleConditionals.clear();
    areaTextStyleConditionals.clear();
    areaIconStyleConditionals.clear();
    areaBorderTextStyleConditionals.clear();
    areaBorderSymbolStyleConditionals.clear();

    areaFillStyleSelectors.clear();
    areaTextStyleSelectors.clear();
    areaIconStyleSelectors.clear();
    areaBorderTextStyleSelectors.clear();
    areaBorderSymbolStyleSelectors.clear();
    areaTypeSets.clear();

    constants.clear();
  }

  bool StyleConfig::RegisterLabelProviderFactory(const std::string& name,
                                                 const LabelProviderFactoryRef& factory)
  {
    if (!factory) {
      return false;
    }

    if (labelFactories.find(name)!=labelFactories.end()) {
      return false;
    }

    labelFactories[name]=factory;

    return true;
  }

  LabelProviderRef StyleConfig::GetLabelProvider(const std::string& name) const
  {
    auto entry=labelFactories.find(name);

    if (entry==labelFactories.end()) {
      return NULL;
    }

    return entry->second->Create(*typeConfig);
  }

  /**
   * Returns 'true', if the given flag exists, else 'false'.
   */
  bool StyleConfig::HasFlag(const std::string& name) const
  {
    return flags.find(name)!=flags.end();
  }

  /**
   * Returns thevalue of the given flag identified by the name of the flag.
   *
   * Asserts, if the flag name is unnown.
   */
  bool StyleConfig::GetFlagByName(const std::string& name) const
  {
    const auto entry=flags.find(name);

    assert(entry!=flags.end());

    return entry->second;
  }

  /**
   * Add the flag with the given value. If the flag already exists, its value
   * gets overwritten.
   */
  void StyleConfig::AddFlag(const std::string& name,
                           bool value)
  {
    flags[name]=value;
  }

  StyleConstantRef StyleConfig::GetConstantByName(const std::string& name) const
  {
    StyleConstantRef result;

    auto entry=constants.find(name);

    if (entry!=constants.end()) {
      result=entry->second;
    }

    return result;
  }

  void StyleConfig::AddConstant(const std::string& name,
                                const StyleConstantRef& variable)
  {
    constants.insert(std::make_pair(name,variable));
  }

  bool StyleConfig::RegisterSymbol(const SymbolRef& symbol)
  {
    std::pair<std::unordered_map<std::string,SymbolRef>::iterator,bool> result;

    result=symbols.insert(std::make_pair(symbol->GetName(),symbol));

    return result.second;
  }

  const SymbolRef& StyleConfig::GetSymbol(const std::string& name) const
  {
    auto entry=symbols.find(name);

    if (entry!=symbols.end()) {
      return entry->second;
    }
    else {
      return emptySymbol;
    }
  }

  void StyleConfig::GetAllNodeTypes(std::list<TypeId>& types)
  {
    for (const auto& type : typeConfig->GetNodeTypes()) {
      types.push_back(type->GetNodeId());
    }
  }

  void StyleConfig::GetAllWayTypes(std::list<TypeId>& types)
  {
    for (const auto& type : typeConfig->GetWayTypes()) {
      types.push_back(type->GetWayId());
    }
  }

  void StyleConfig::GetAllAreaTypes(std::list<TypeId>& types)
  {
    for (const auto& type : typeConfig->GetAreaTypes()) {
      types.push_back(type->GetAreaId());
    }
  }

  template <class S, class A>
  void GetMaxLevelInConditionals(const std::list<ConditionalStyle<S,A> >& conditionals,
                                 size_t& maxLevel)
  {
    for (const auto& conditional : conditionals) {
      maxLevel=std::max(maxLevel,conditional.filter.GetMinLevel()+1);

      if (conditional.filter.HasMaxLevel()) {
        maxLevel=std::max(maxLevel,conditional.filter.GetMaxLevel()+1);
      }
    }
  }

  template <class S, class A>
  void CalculateUsedTypes(const TypeConfig& typeConfig,
                          const std::list<ConditionalStyle<S,A> >& conditionals,
                          size_t maxLevel,
                          std::vector<TypeInfoSet>& typeSets)
  {
    for (size_t level=0;
        level<maxLevel;
        ++level) {
      for (const auto& conditional : conditionals) {
        for (const auto& type : typeConfig.GetTypes()) {
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

          typeSets[level].Set(type);
        }
      }
    }
  }

  template <class S, class A>
  void SortInConditionals(const TypeConfig& typeConfig,
                          const std::list<ConditionalStyle<S,A> >& conditionals,
                          size_t maxLevel,
                          std::vector<std::vector<std::list<StyleSelector<S,A> > > >& selectors)
  {
    selectors.resize(typeConfig.GetTypeCount());

    for (auto& selector : selectors) {
      selector.resize(maxLevel+1);
    }

    for (const auto& conditional : conditionals) {
      StyleSelector<S,A> selector(conditional.filter,conditional.style);

      for (const auto& type : typeConfig.GetTypes()) {
        if (!conditional.filter.HasType(type)) {
          continue;
        }

        size_t minLvl=conditional.filter.GetMinLevel();
        size_t maxLvl=conditional.filter.HasMaxLevel() ? conditional.filter.GetMaxLevel() : maxLevel;

        for (size_t level=minLvl; level<=maxLvl; level++) {
          selectors[type->GetIndex()][level].push_back(selector);
        }
      }
    }

    for (auto& selector : selectors) {
      for (size_t level=0; level<selector.size(); level++) {
        if (selector[level].size()>=2) {
          // If two consecutive conditions are equal, one can be removed and the style can get merged
          typename std::list<StyleSelector<S,A> >::iterator prevSelector=selector[level].begin();
          typename std::list<StyleSelector<S,A> >::iterator curSelector=prevSelector;

          curSelector++;

          while (curSelector!=selector[level].end()) {
            if (prevSelector->criteria==curSelector->criteria) {
              prevSelector->attributes.insert(curSelector->attributes.begin(),
                                              curSelector->attributes.end());
              prevSelector->style=std::make_shared<S>(*prevSelector->style);
              prevSelector->style->CopyAttributes(*curSelector->style,
                                                  curSelector->attributes);

              curSelector=selector[level].erase(curSelector);
            }
            else {
              prevSelector=curSelector;
              curSelector++;
            }
          }
        }

        // If there is only one conditional and it is not visible, we can remove it
        if (selector[level].size()==1 &&
            !selector[level].front().style->IsVisible()) {
          selector[level].clear();
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

    std::unordered_map<std::string,std::list<TextConditionalStyle> > textStyleBySlot;

    for (auto& conditional : nodeTextStyleConditionals) {
      textStyleBySlot[conditional.style.style->GetSlot()].push_back(conditional);
    }

    nodeTextStyleSelectors.resize(textStyleBySlot.size());

    size_t idx=0;
    for (const auto& entry : textStyleBySlot) {
      SortInConditionals(*typeConfig,
                         entry.second,
                         maxLevel,
                         nodeTextStyleSelectors[idx]);

      idx++;
    }

    SortInConditionals(*typeConfig,
                       nodeIconStyleConditionals,
                       maxLevel,
                       nodeIconStyleSelectors);

    nodeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      nodeTypeSets.push_back(TypeInfoSet(*typeConfig));
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

    std::unordered_map<std::string,std::list<LineConditionalStyle> > lineStyleBySlot;

    for (auto& conditional : wayLineStyleConditionals) {
      lineStyleBySlot[conditional.style.style->GetSlot()].push_back(conditional);
    }

    wayLineStyleSelectors.resize(lineStyleBySlot.size());

    size_t idx=0;
    for (const auto& entry : lineStyleBySlot) {
      SortInConditionals(*typeConfig,
                         entry.second,
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

    wayTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      wayTypeSets.push_back(TypeInfoSet(*typeConfig));
    }

    CalculateUsedTypes(*typeConfig,
                       wayLineStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    CalculateUsedTypes(*typeConfig,
                       wayPathTextStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    CalculateUsedTypes(*typeConfig,
                       wayPathSymbolStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    CalculateUsedTypes(*typeConfig,
                       wayPathShieldStyleConditionals,
                       maxLevel,
                       wayTypeSets);

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
    GetMaxLevelInConditionals(areaBorderTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaBorderSymbolStyleConditionals,
                              maxLevel);

    SortInConditionals(*typeConfig,
                       areaFillStyleConditionals,
                       maxLevel,
                       areaFillStyleSelectors);

    std::unordered_map<std::string,std::list<TextConditionalStyle> > textStyleBySlot;

    for (auto& conditional : areaTextStyleConditionals) {
      textStyleBySlot[conditional.style.style->GetSlot()].push_back(conditional);
    }

    areaTextStyleSelectors.resize(textStyleBySlot.size());

    size_t idx=0;
    for (const auto& entry : textStyleBySlot) {
      SortInConditionals(*typeConfig,
                         entry.second,
                         maxLevel,
                         areaTextStyleSelectors[idx]);

      idx++;
    }

    SortInConditionals(*typeConfig,
                       areaIconStyleConditionals,
                       maxLevel,
                       areaIconStyleSelectors);

    SortInConditionals(*typeConfig,
                       areaBorderTextStyleConditionals,
                       maxLevel,
                       areaBorderTextStyleSelectors);

    SortInConditionals(*typeConfig,
                       areaBorderSymbolStyleConditionals,
                       maxLevel,
                       areaBorderSymbolStyleSelectors);

    areaTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      areaTypeSets.push_back(TypeInfoSet(*typeConfig));
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
    CalculateUsedTypes(*typeConfig,
                       areaBorderTextStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaBorderSymbolStyleConditionals,
                       maxLevel,
                       areaTypeSets);

    areaFillStyleConditionals.clear();
    areaTextStyleConditionals.clear();
    areaIconStyleConditionals.clear();
    areaBorderTextStyleConditionals.clear();
    areaBorderSymbolStyleConditionals.clear();
  }

  void StyleConfig::PostprocessIconId()
  {
    std::unordered_map<std::string,size_t> symbolIdMap;
    size_t                                 nextId=1;

    for (auto& typeSelector : areaIconStyleSelectors) {
      for (auto& levelSelector : typeSelector) {
        for (auto& selector : levelSelector) {
          if (!selector.style->GetIconName().empty()) {
            auto entry=symbolIdMap.find(selector.style->GetIconName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector.style->GetIconName(),nextId));

              selector.style->SetIconId(nextId);

              nextId++;
            }
            else {
              selector.style->SetIconId(entry->second);
            }
          }
        }
      }
    }

    for (auto& typeSelector: nodeIconStyleSelectors) {
      for (auto& levelSelector : typeSelector) {
        for (auto& selector : levelSelector) {
          if (!selector.style->GetIconName().empty()) {
            auto entry=symbolIdMap.find(selector.style->GetIconName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector.style->GetIconName(),nextId));

              selector.style->SetIconId(nextId);

              nextId++;
            }
            else {
              selector.style->SetIconId(entry->second);
            }
          }
        }
      }
    }
  }

  void StyleConfig::PostprocessPatternId()
  {
    std::unordered_map<std::string,size_t> symbolIdMap;
    size_t                                 nextId=1;

    for (auto& typeSelector : areaFillStyleSelectors) {
      for (auto& levelSelector: typeSelector) {
        for (auto& selector : levelSelector) {
          if (!selector.style->GetPatternName().empty()) {
            auto entry=symbolIdMap.find(selector.style->GetPatternName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector.style->GetPatternName(),nextId));

              selector.style->SetPatternId(nextId);

              nextId++;
            }
            else {
              selector.style->SetPatternId(entry->second);
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

  StyleConfig& StyleConfig::SetWayPrio(const TypeInfoRef& type,
                                       size_t prio)
  {
    if (wayPrio.size()<=type->GetIndex()) {
      wayPrio.resize(type->GetIndex()+1,
                     std::numeric_limits<size_t>::max());
    }

    wayPrio[type->GetIndex()]=prio;

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

  void StyleConfig::AddAreaBorderTextStyle(const StyleFilter& filter,
                                           PathTextPartialStyle& style)
  {
    PathTextConditionalStyle conditional(filter,style);

    areaBorderTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaBorderSymbolStyle(const StyleFilter& filter,
                                             PathSymbolPartialStyle& style)
  {
    PathSymbolConditionalStyle conditional(filter,style);

    areaBorderSymbolStyleConditionals.push_back(conditional);
  }

  void StyleConfig::GetNodeTypesWithMaxMag(const Magnification& maxMag,
                                           TypeInfoSet& types) const
  {
    if (!nodeTypeSets.empty()) {
      types=nodeTypeSets[std::min((size_t)maxMag.GetLevel(),nodeTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetWayTypesWithMaxMag(const Magnification& maxMag,
                                          TypeInfoSet& types) const
  {
    if (!wayTypeSets.empty()) {
      types=wayTypeSets[std::min((size_t)maxMag.GetLevel(),wayTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetAreaTypesWithMaxMag(const Magnification& maxMag,
                                           TypeInfoSet& types) const
  {
    if (!areaTypeSets.empty()) {
      types=areaTypeSets[std::min((size_t)maxMag.GetLevel(),areaTypeSets.size()-1)];
    }
  }

  /**
   * Get the style data based on the given features of an object,
   * a given style (S) and its style attributes (A).
   */
  template <class S, class A>
  void GetFeatureStyle(const StyleResolveContext& context,
                       const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                       const FeatureValueBuffer& buffer,
                       const Projection& projection,
                       std::shared_ptr<S>& style)
  {
    bool   fastpath=false;
    bool   composed=false;
    size_t level=projection.GetMagnification().GetLevel();
    double meterInPixel=projection.GetMeterInPixel();
    double meterInMM=projection.GetMeterInMM();

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    style=NULL;

    for (const auto& selector : styleSelectors[level]) {
      if (!selector.criteria.Matches(context,
                                     buffer,
                                     meterInPixel,
                                     meterInMM)) {
        continue;
      }

      if (!style) {
        style=selector.style;
        fastpath=true;

        continue;
      }

      if (fastpath) {
        style=std::make_shared<S>(*style);
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

  bool StyleConfig::HasNodeTextStyles(const TypeInfoRef& type,
                                      const Magnification& magnification) const
  {
    auto level=magnification.GetLevel();

    for (size_t slot=0; slot<nodeTextStyleSelectors.size(); slot++) {
      if (level>=nodeTextStyleSelectors[slot][type->GetIndex()].size()) {
        level=nodeTextStyleSelectors[slot][type->GetIndex()].size()-1;
      }

      if (!nodeTextStyleSelectors[slot][type->GetIndex()][level].empty()) {
        return true;
      }
    }

    return false;
  }

  void StyleConfig::GetNodeTextStyles(const FeatureValueBuffer& buffer,
                                      const Projection& projection,
                                      std::vector<TextStyleRef>& textStyles) const
  {
    TextStyleRef style;

    textStyles.clear();
    textStyles.reserve(nodeTextStyleSelectors.size());

    for (size_t slot=0; slot<nodeTextStyleSelectors.size(); slot++) {
      style=NULL;

      GetFeatureStyle(styleResolveContext,
                      nodeTextStyleSelectors[slot][buffer.GetType()->GetIndex()],
                      buffer,
                      projection,
                      style);

      if (style) {
        textStyles.push_back(style);
      }
    }
  }

  void StyleConfig::GetNodeIconStyle(const FeatureValueBuffer& buffer,
                                     const Projection& projection,
                                     IconStyleRef& iconStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    nodeIconStyleSelectors[buffer.GetType()->GetIndex()],
                    buffer,
                    projection,
                    iconStyle);
  }

  void StyleConfig::GetWayLineStyles(const FeatureValueBuffer& buffer,
                                     const Projection& projection,
                                     std::vector<LineStyleRef>& lineStyles) const
  {
    LineStyleRef style;

    lineStyles.clear();
    lineStyles.reserve(wayLineStyleSelectors.size());

    for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
      style=NULL;

      GetFeatureStyle(styleResolveContext,
                      wayLineStyleSelectors[slot][buffer.GetType()->GetIndex()],
                      buffer,
                      projection,
                      style);

      if (style) {
        lineStyles.push_back(style);
      }
    }
  }

  void StyleConfig::GetWayPathTextStyle(const FeatureValueBuffer& buffer,
                                        const Projection& projection,
                                        PathTextStyleRef& pathTextStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    wayPathTextStyleSelectors[buffer.GetType()->GetIndex()],
                    buffer,
                    projection,
                    pathTextStyle);
  }

  void StyleConfig::GetWayPathSymbolStyle(const FeatureValueBuffer& buffer,
                                          const Projection& projection,
                                          PathSymbolStyleRef& pathSymbolStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    wayPathSymbolStyleSelectors[buffer.GetType()->GetIndex()],
                    buffer,
                    projection,
                    pathSymbolStyle);
  }

  void StyleConfig::GetWayPathShieldStyle(const FeatureValueBuffer& buffer,
                                          const Projection& projection,
                                          PathShieldStyleRef& pathShieldStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    wayPathShieldStyleSelectors[buffer.GetType()->GetIndex()],
                    buffer,
                    projection,
                    pathShieldStyle);
  }

  void StyleConfig::GetAreaFillStyle(const TypeInfoRef& type,
                                     const FeatureValueBuffer& buffer,
                                     const Projection& projection,
                                     FillStyleRef& fillStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaFillStyleSelectors[type->GetIndex()],
                    buffer,
                    projection,
                    fillStyle);
  }

  bool StyleConfig::HasAreaTextStyles(const TypeInfoRef& type,
                                      const Magnification& magnification) const
  {
    auto level=magnification.GetLevel();

    for (size_t slot=0; slot<areaTextStyleSelectors.size(); slot++) {
      if (level>=areaTextStyleSelectors[slot][type->GetIndex()].size()) {
        level=areaTextStyleSelectors[slot][type->GetIndex()].size()-1;
      }

      if (!areaTextStyleSelectors[slot][type->GetIndex()][level].empty()) {
        return true;
      }
    }

    return false;
  }

  void StyleConfig::GetAreaTextStyles(const TypeInfoRef& type,
                                      const FeatureValueBuffer& buffer,
                                      const Projection& projection,
                                      std::vector<TextStyleRef>& textStyles) const
  {
    TextStyleRef style;

    textStyles.clear();
    textStyles.reserve(areaTextStyleSelectors.size());

    for (size_t slot=0; slot<areaTextStyleSelectors.size(); slot++) {
      style=NULL;

      GetFeatureStyle(styleResolveContext,
                      areaTextStyleSelectors[slot][type->GetIndex()],
                      buffer,
                      projection,
                      style);

      if (style) {
        textStyles.push_back(style);
      }
    }
  }

  void StyleConfig::GetAreaIconStyle(const TypeInfoRef& type,
                                     const FeatureValueBuffer& buffer,
                                     const Projection& projection,
                                     IconStyleRef& iconStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaIconStyleSelectors[type->GetIndex()],
                    buffer,
                    projection,
                    iconStyle);
  }

  void StyleConfig::GetAreaBorderTextStyle(const TypeInfoRef& type,
                                           const FeatureValueBuffer& buffer,
                                           const Projection& projection,
                                           PathTextStyleRef& pathTextStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaBorderTextStyleSelectors[type->GetIndex()],
                    buffer,
                    projection,
                    pathTextStyle);
  }

  void StyleConfig::GetAreaBorderSymbolStyle(const TypeInfoRef& type,
                                             const FeatureValueBuffer& buffer,
                                             const Projection& projection,
                                             PathSymbolStyleRef& pathSymbolStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaBorderSymbolStyleSelectors[type->GetIndex()],
                    buffer,
                    projection,
                    pathSymbolStyle);
  }

  void StyleConfig::GetLandFillStyle(const Projection& projection,
                                     FillStyleRef& fillStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaFillStyleSelectors[tileLandBuffer.GetType()->GetIndex()],
                    tileLandBuffer,
                    projection,
                    fillStyle);
  }

  void StyleConfig::GetSeaFillStyle(const Projection& projection,
                                    FillStyleRef& fillStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaFillStyleSelectors[tileSeaBuffer.GetType()->GetIndex()],
                    tileSeaBuffer,
                    projection,
                    fillStyle);
  }

  void StyleConfig::GetCoastFillStyle(const Projection& projection,
                                      FillStyleRef& fillStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaFillStyleSelectors[tileCoastBuffer.GetType()->GetIndex()],
                    tileCoastBuffer,
                    projection,
                    fillStyle);
  }

  void StyleConfig::GetUnknownFillStyle(const Projection& projection,
                                        FillStyleRef& fillStyle) const
  {
    GetFeatureStyle(styleResolveContext,
                    areaFillStyleSelectors[tileUnknownBuffer.GetType()->GetIndex()],
                    tileUnknownBuffer,
                    projection,
                    fillStyle);
  }

  void StyleConfig::GetCoastlineLineStyle(const Projection& projection,
                                          LineStyleRef& lineStyle) const
  {
    for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
      GetFeatureStyle(styleResolveContext,
                      wayLineStyleSelectors[slot][coastlineBuffer.GetType()->GetIndex()],
                      coastlineBuffer,
                      projection,
                      lineStyle);

      if (lineStyle) {
        return;
      }
    }
  }

  void StyleConfig::GetOSMTileBorderLineStyle(const Projection& projection,
                                              LineStyleRef& lineStyle) const
  {
    for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
      GetFeatureStyle(styleResolveContext,
                      wayLineStyleSelectors[slot][osmTileBorderBuffer.GetType()->GetIndex()],
                      osmTileBorderBuffer,
                      projection,
                      lineStyle);

      if (lineStyle) {
        return;
      }
    }
  }

  void StyleConfig::GetOSMSubTileBorderLineStyle(const Projection& projection,
                                                 LineStyleRef& lineStyle) const
  {
    for (size_t slot=0; slot<wayLineStyleSelectors.size(); slot++) {
      GetFeatureStyle(styleResolveContext,
                      wayLineStyleSelectors[slot][osmSubTileBorderBuffer.GetType()->GetIndex()],
                      osmSubTileBorderBuffer,
                      projection,
                      lineStyle);

      if (lineStyle) {
        return;
      }
    }
  }

  void StyleConfig::GetNodeTextStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<TextStyleSelector>& selectors) const
  {
    selectors.clear();

    for (const auto& slotEntry : nodeTextStyleSelectors) {
      size_t l=level;

      if (l>=slotEntry[type->GetIndex()].size()) {
        l=slotEntry[type->GetIndex()].size()-1;
      }

      for (const auto& selector : slotEntry[type->GetIndex()][l]) {
        selectors.push_back(selector);
      }
    }
  }

  void StyleConfig::GetAreaFillStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<FillStyleSelector>& selectors) const
  {
    selectors.clear();

    if (level>=areaFillStyleSelectors[type->GetIndex()].size()) {
      level=areaFillStyleSelectors[type->GetIndex()].size()-1;
    }

    for (const auto& selector : areaFillStyleSelectors[type->GetIndex()][level]) {
      selectors.push_back(selector);
    }
  }

  void StyleConfig::GetAreaTextStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<TextStyleSelector>& selectors) const
  {
    selectors.clear();

    for (const auto& slotEntry : areaTextStyleSelectors) {
      size_t l=level;

      if (l>=slotEntry[type->GetIndex()].size()) {
        l=slotEntry[type->GetIndex()].size()-1;
      }

      for (const auto& selector : slotEntry[type->GetIndex()][l]) {
        selectors.push_back(selector);
      }
    }
  }

  bool StyleConfig::Load(const std::string& styleFile)
  {
    StopClock  timer;
    FileOffset fileSize;
    FILE*      file;
    bool       success=false;

    try {
      Reset();

      fileSize=GetFileSize(styleFile);

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

        return false;
      }

      fclose(file);

      oss::Scanner *scanner=new oss::Scanner(content,
                                             fileSize);
      oss::Parser  *parser=new oss::Parser(scanner,
                                           *this);

      delete [] content;

      parser->Parse();

      success=!parser->errors->hasErrors;

      errors.clear();
      if (!success) {
        for (const auto& err : parser->errors->errors) {
          switch(err.type) {
          case oss::Errors::Err::Symbol:
            errors.push_back(std::to_string(err.line)+","+std::to_string(err.column)+std::string(" Symbol:")+err.text);
            break;
          case oss::Errors::Err::Error:
            errors.push_back(std::to_string(err.line)+","+std::to_string(err.column)+std::string(" Error:")+err.text);
            break;
          case oss::Errors::Err::Warning:
            errors.push_back(std::to_string(err.line)+","+std::to_string(err.column)+std::string(" Warning:")+err.text);
            break;
          case oss::Errors::Err::Exception:
            errors.push_back(std::to_string(err.line)+","+std::to_string(err.column)+std::string(" Exception:")+err.text);
            break;
          default:
            break;
          }
        }
      }

      delete parser;
      delete scanner;

      Postprocess();

      timer.Stop();

      log.Debug() << "Opening StyleConfig: " << timer.ResultString();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
    }

    return success;
  }

  const std::list<std::string>& StyleConfig::GetErrors()
  {
    return errors;
  }
}

