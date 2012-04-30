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

#include <limits>
#include <set>

namespace osmscout {

  LineStyle::LineStyle()
   : lineColor(1,1,1),
     alternateColor(0.5,0.5,0.5),
     outlineColor(0.75,0.75,0.75),
     gapColor(1,0,0,0),
     minWidth(1),
     width(0),
     fixedWidth(false),
     outline(0)
  {
    // no code
  }

  LineStyle& LineStyle::SetLineColor(const Color& color)
  {
    this->lineColor=color;
    this->alternateColor=color;

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

  LineStyle& LineStyle::SetMinWidth(double value)
  {
    minWidth=value;

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

  LineStyle& LineStyle::SetOutline(double value)
  {
    outline=value;

    return *this;
  }

  LineStyle& LineStyle::AddDashValue(double dashValue)
  {
    dash.push_back(dashValue);

    return *this;
  }


  FillStyle::FillStyle()
   : style(none),
     fillColor(1,0,0),
     patternId(0),
     patternMinMag(magWorld),
     borderColor(1,0,0),
     borderWidth(0.0)
  {
    // no code
  }

  FillStyle& FillStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
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

  FillStyle& FillStyle::AddBorderDashValue(double dashValue)
  {
    borderDash.push_back(dashValue);

    return *this;
  }

  LabelStyle::LabelStyle()
   : style(none),
     priority(0),
     minMag(magWorld),
     scaleAndFadeMag((Mag)1000000),
     maxMag((Mag)1000000),
     size(1),
     textColor(0,0,0),
     bgColor(1,1,1),
     borderColor(0,0,0)
  {
    // no code
  }

  LabelStyle& LabelStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  LabelStyle& LabelStyle::SetPriority(uint8_t priority)
  {
    this->priority=priority;

    return *this;
  }

  LabelStyle& LabelStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  LabelStyle& LabelStyle::SetScaleAndFadeMag(Mag mag)
  {
    this->scaleAndFadeMag=mag;

    return *this;
  }

  LabelStyle& LabelStyle::SetMaxMag(Mag mag)
  {
    this->maxMag=mag;

    return *this;
  }

  LabelStyle& LabelStyle::SetSize(double size)
  {
    this->size=size;

    return *this;
  }

  LabelStyle& LabelStyle::SetTextColor(const Color& color)
  {
    this->textColor=color;

    return *this;
  }

  LabelStyle& LabelStyle::SetBgColor(const Color& color)
  {
    this->bgColor=color;

    return *this;
  }

  LabelStyle& LabelStyle::SetBorderColor(const Color& color)
  {
    this->borderColor=color;

    return *this;
  }

  SymbolStyle::SymbolStyle()
   : style(none),
     minMag(magWorld),
     size(8),
     fillColor(1,0,0)
  {
    // no code
  }

  SymbolStyle& SymbolStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  SymbolStyle& SymbolStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  SymbolStyle& SymbolStyle::SetSize(double size)
  {
    this->size=size;

    return *this;
  }

  SymbolStyle& SymbolStyle::SetFillColor(const Color& color)
  {
    fillColor=color;

    return *this;
  }

  IconStyle::IconStyle()
   : id(0),
     minMag(magWorld)
  {
    // no code
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

  IconStyle& IconStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  StyleConfig::StyleConfig(TypeConfig* typeConfig)
   : typeConfig(typeConfig)
  {
    // no code
  }

  StyleConfig::~StyleConfig()
  {
    // no code
  }

  void StyleConfig::ReserveSpaceForNodeType(TypeId type)
  {
    if (type>=nodeLabelStyles.size()) {
      nodeSymbolStyles.resize(type+1,NULL);
      nodeRefLabelStyles.resize(type+1,NULL);
      nodeLabelStyles.resize(type+1,NULL);
      nodeIconStyles.resize(type+1,NULL);
    }
  }

  void StyleConfig::ReserveSpaceForWayType(TypeId type)
  {
    if (type>=wayLineStyles.size()) {
      wayPrio.resize(type+1,std::numeric_limits<size_t>::max());
      wayMag.resize(type+1,magVeryClose);
      wayLineStyles.resize(type+1);
      wayRefLabelStyles.resize(type+1);
      wayNameLabelStyles.resize(type+1);
    }
  }

  void StyleConfig::ReserveSpaceForAreaType(TypeId type)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }
  }

  void StyleConfig::PostprocessNodes()
  {
    size_t maxLevel=0;
    for (size_t type=0; type<nodeLabelStyles.size(); type++) {
      if (nodeLabelStyles[type].Valid()) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeLabelStyles[type]->GetMinMag()));
      }
      else if (nodeRefLabelStyles[type].Valid()) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeRefLabelStyles[type]->GetMinMag()));
      }
      else if (nodeSymbolStyles[type].Valid()) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeSymbolStyles[type]->GetMinMag()));
      }
      else if (nodeIconStyles[type].Valid()) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeIconStyles[type]->GetMinMag()));
      }
    }

    nodeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      nodeTypeSets.push_back(TypeSet(*typeConfig));
    }

    for (size_t level=0;
        level<nodeTypeSets.size();
        ++level) {
      for (size_t type=0; type<nodeLabelStyles.size(); type++) {
        if (nodeLabelStyles[type].Valid() &&
            MagToLevel(nodeLabelStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }

        if (nodeRefLabelStyles[type].Valid() &&
            MagToLevel(nodeRefLabelStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }

        if (nodeSymbolStyles[type].Valid() &&
            MagToLevel(nodeSymbolStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }

        if (nodeIconStyles[type].Valid() &&
            MagToLevel(nodeIconStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }
      }
    }
  }

  void StyleConfig::PostprocessWays()
  {
    size_t maxLevel=0;
    for (size_t i=0; i<wayLineStyles.size(); i++) {
      maxLevel=std::max(maxLevel,MagToLevel(wayMag[i]));
    }

    wayTypeSets.resize(maxLevel+1);

    std::set<size_t> prios;

    for (size_t i=0; i<wayLineStyles.size(); i++) {
      prios.insert(wayPrio[i]);
    }

    for (size_t level=0;
        level<wayTypeSets.size();
        ++level) {
      for (std::set<size_t>::const_iterator prio=prios.begin();
          prio!=prios.end();
          ++prio) {
        TypeSet typeSet(*typeConfig);

        for (size_t i=0; i<wayLineStyles.size(); i++) {
          if (wayPrio[i]==*prio &&
              MagToLevel(wayMag[i])<=level) {
            typeSet.SetType(i);
          }
        }

        // TODO: Is type set not empty?
        wayTypeSets[level].push_back(typeSet);
      }
    }
  }

  void StyleConfig::PostprocessAreas()
  {
    size_t maxLevel=0;
    for (size_t i=0; i<areaFillStyles.size(); i++) {
      maxLevel=std::max(maxLevel,MagToLevel(areaMag[i]));
    }

    areaTypeSets.reserve(maxLevel);

    for (size_t i=0; i<maxLevel; i++) {
      areaTypeSets.push_back(TypeSet(*typeConfig));
    }

    for (size_t level=0;
        level<areaTypeSets.size();
        ++level) {
      for (size_t i=0; i<areaFillStyles.size(); i++) {
        if (MagToLevel(areaMag[i])<=level) {
          areaTypeSets[level].SetType(i);
        }
      }
    }
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

  StyleConfig& StyleConfig::SetNodeSymbolStyle(TypeId type,
                                               const SymbolStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeSymbolStyles[type];
    nodeSymbolStyles[type]=new SymbolStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeLabelStyle(TypeId type,
                                              const LabelStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeLabelStyles[type];
    nodeLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeRefLabelStyle(TypeId type,
                                                 const LabelStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeRefLabelStyles[type];
    nodeRefLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeIconStyle(TypeId type,
                                             const IconStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeIconStyles[type];
    nodeIconStyles[type]=new IconStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayPrio(TypeId type, size_t prio)
  {
    ReserveSpaceForWayType(type);

    wayPrio[type]=prio;

    return *this;
  }

  StyleConfig& StyleConfig::SetWayMag(TypeId type, Mag mag)
  {
    ReserveSpaceForWayType(type);

    wayMag[type]=mag;

    return *this;
  }

  StyleConfig& StyleConfig::SetWayLineStyle(TypeId type,
                                            const LineStyle& style)
  {
    ReserveSpaceForWayType(type);

    delete wayLineStyles[type];
    wayLineStyles[type]=new LineStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayRefLabelStyle(TypeId type,
                                                const LabelStyle& style)
  {
    ReserveSpaceForWayType(type);

    delete wayRefLabelStyles[type];
    wayRefLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayNameLabelStyle(TypeId type,
                                                 const LabelStyle& style)
  {
    ReserveSpaceForWayType(type);

    delete wayNameLabelStyles[type];
    wayNameLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaMag(TypeId type, Mag mag)
  {
    ReserveSpaceForAreaType(type);

    areaMag[type]=mag;

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaFillStyle(TypeId type,
                                             const FillStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaFillStyles[type];
    areaFillStyles[type]=new FillStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaLabelStyle(TypeId type,
                                              const LabelStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaLabelStyles[type];
    areaLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaSymbolStyle(TypeId type,
                                               const SymbolStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaSymbolStyles[type];
    areaSymbolStyles[type]=new SymbolStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaIconStyle(TypeId type,
                                             const IconStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaIconStyles[type];
    areaIconStyles[type]=new IconStyle(style);

    return *this;
  }

  void StyleConfig::GetNodeTypesWithMaxMag(double maxMag,
                                           TypeSet& types) const
  {
    types=nodeTypeSets[std::min(MagToLevel(maxMag),nodeTypeSets.size()-1)];
  }

  void StyleConfig::GetWayTypesByPrioWithMaxMag(double maxMag,
                                             std::vector<TypeSet>& types) const
  {
    types=wayTypeSets[std::min(MagToLevel(maxMag),wayTypeSets.size()-1)];
  }

  void StyleConfig::GetAreaTypesWithMaxMag(double maxMag,
                                           TypeSet& types) const
  {
    types=areaTypeSets[std::min(MagToLevel(maxMag),areaTypeSets.size()-1)];
  }
}

