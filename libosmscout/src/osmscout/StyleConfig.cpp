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
#include <iostream>
namespace osmscout {

  LineStyle::LineStyle()
   : lineR(1),
     lineG(1),
     lineB(1),
     lineA(1),
     alternateR(0.5),
     alternateG(0.5),
     alternateB(0.5),
     alternateA(1),
     outlineR(0.75),
     outlineG(0.75),
     outlineB(0.75),
     outlineA(1),
     minPixel(2),
     width(0),
     fixedWidth(false),
     outline(0)
  {
    // no code
  }

  LineStyle& LineStyle::SetLineColor(double r, double g, double b, double a)
  {
    lineR=r;
    lineG=g;
    lineB=b;
    lineA=a;

    alternateR=r;
    alternateG=g;
    alternateB=b;
    alternateA=a;

    return *this;
  }

  LineStyle& LineStyle::SetAlternateColor(double r, double g, double b, double a)
  {
    alternateR=r;
    alternateG=g;
    alternateB=b;
    alternateA=a;

    return *this;
  }

  LineStyle& LineStyle::SetOutlineColor(double r, double g, double b, double a)
  {
    outlineR=r;
    outlineG=g;
    outlineB=b;
    outlineA=a;

    return *this;
  }

  LineStyle& LineStyle::SetMinPixel(double value)
  {
    minPixel=value;

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
     layer(0),
     fillR(1),
     fillG(0),
     fillB(0),
     fillA(1)
  {
    // no code
  }

  FillStyle& FillStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  FillStyle& FillStyle::SetLayer(int layer)
  {
    this->layer=layer;

    return *this;
  }

  FillStyle& FillStyle::SetColor(double r, double g, double b, double a)
  {
    fillR=r;
    fillG=g;
    fillB=b;
    fillA=a;

    return *this;
  }

  PatternStyle::PatternStyle()
   : layer(0),
     id(0)
  {
    // no code
  }

  PatternStyle& PatternStyle::SetLayer(int layer)
  {
    this->layer=layer;

    return *this;
  }

  PatternStyle& PatternStyle::SetId(size_t id)
  {
    this->id=id;

    return *this;
  }

  PatternStyle& PatternStyle::SetPattern(const std::string& pattern)
  {
    this->pattern=pattern;

    return *this;
  }

  PatternStyle& PatternStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  LabelStyle::LabelStyle()
   : style(none),
     priority(0),
     minMag(magWorld),
     scaleAndFadeMag((Mag)1000000),
     maxMag((Mag)1000000),
     size(1),
     textR(0),
     textG(0),
     textB(0),
     textA(1),
     bgR(1),
     bgG(1),
     bgB(1),
     bgA(1),
     borderR(0),
     borderG(0),
     borderB(0),
     borderA(1)
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

  LabelStyle& LabelStyle::SetTextColor(double r, double g, double b, double a)
  {
    this->textR=r;
    this->textG=g;
    this->textB=b;
    this->textA=a;

    return *this;
  }

  LabelStyle& LabelStyle::SetBgColor(double r, double g, double b, double a)
  {
    this->bgR=r;
    this->bgG=g;
    this->bgB=b;
    this->bgA=a;

    return *this;
  }

  LabelStyle& LabelStyle::SetBorderColor(double r, double g, double b, double a)
  {
    this->borderR=r;
    this->borderG=g;
    this->borderB=b;
    this->borderA=a;

    return *this;
  }

  SymbolStyle::SymbolStyle()
   : style(none),
     minMag(magWorld),
     size(8),
     fillR(1),
     fillG(0),
     fillB(0),
     fillA(1)
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

  SymbolStyle& SymbolStyle::SetFillColor(double r, double g, double b, double a)
  {
    fillR=r;
    fillG=g;
    fillB=b;
    fillA=a;

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
    for (size_t i=0; i<nodeSymbolStyles.size(); i++) {
      delete nodeSymbolStyles[i];
    }

    for (size_t i=0; i<nodeRefLabelStyles.size(); i++) {
      delete nodeRefLabelStyles[i];
    }

    for (size_t i=0; i<nodeLabelStyles.size(); i++) {
      delete nodeLabelStyles[i];
    }

    for (size_t i=0; i<nodeIconStyles.size(); i++) {
      delete nodeIconStyles[i];
    }

    for (size_t i=0; i<wayLineStyles.size(); i++) {
      delete wayLineStyles[i];
    }

    for (size_t i=0; i<wayRefLabelStyles.size(); i++) {
      delete wayRefLabelStyles[i];
    }

    for (size_t i=0; i<wayNameLabelStyles.size(); i++) {
      delete wayNameLabelStyles[i];
    }

    for (size_t i=0; i<areaFillStyles.size(); i++) {
      delete areaFillStyles[i];
    }

    for (size_t i=0; i<areaBuildingFillStyles.size(); i++) {
      delete areaBuildingFillStyles[i];
    }

    for (size_t i=0; i<areaPatternStyles.size(); i++) {
      delete areaPatternStyles[i];
    }

    for (size_t i=0; i<areaSymbolStyles.size(); i++) {
      delete areaSymbolStyles[i];
    }

    for (size_t i=0; i<areaLabelStyles.size(); i++) {
      delete areaLabelStyles[i];
    }

    for (size_t i=0; i<areaBorderStyles.size(); i++) {
      delete areaBorderStyles[i];
    }

    for (size_t i=0; i<areaIconStyles.size(); i++) {
      delete areaIconStyles[i];
    }
  }

  void StyleConfig::Postprocess()
  {
    std::set<size_t > prios;

    for (size_t i=0; i<wayLineStyles.size() && i<wayPrio.size(); i++) {
      if (wayLineStyles[i]!=NULL) {
        prios.insert(wayPrio[i]);
      }
    }

    priorities.clear();
    priorities.reserve(prios.size());
    for (std::set<size_t>::const_iterator prio=prios.begin();
         prio!=prios.end();
         ++prio) {
      priorities.push_back(*prio);
    }

    wayTypesByPrio.clear();
    wayTypesByPrio.reserve(priorities.size());
    for (size_t p=0; p<priorities.size(); p++) {
      for (size_t i=0; i<wayLineStyles.size() && i<wayPrio.size(); i++) {
        if (wayLineStyles[i]!=NULL && wayPrio[i]==priorities[p]) {
          wayTypesByPrio.push_back(i);
        }
      }
    }

    std::set<Mag> magnifications;
    for (size_t i=0; i<nodeSymbolStyles.size(); i++) {
      if (nodeLabelStyles[i]!=NULL) {
        magnifications.insert(nodeLabelStyles[i]->GetMinMag());
      }

      if (nodeSymbolStyles[i]!=NULL) {
        magnifications.insert(nodeSymbolStyles[i]->GetMinMag());
      }
    }

    std::map<Mag,std::set<TypeId> > nodeTypesByMag;

    for (std::set<Mag>::const_iterator mag=magnifications.begin();
         mag!=magnifications.end();
         ++mag) {
      for (size_t i=0; i<nodeSymbolStyles.size() || i<nodeLabelStyles.size(); i++) {
        if (nodeLabelStyles[i]!=NULL && *mag>=nodeLabelStyles[i]->GetMinMag()) {
          nodeTypesByMag[*mag].insert(i);
        }
        if (nodeSymbolStyles[i]!=NULL && *mag>=nodeSymbolStyles[i]->GetMinMag()) {
          nodeTypesByMag[*mag].insert(i);
        }
      }
    }
  }

  TypeConfig* StyleConfig::GetTypeConfig() const
  {
    return typeConfig;
  }

  StyleConfig& StyleConfig::SetWayPrio(TypeId type, size_t prio)
  {
    if (type>=wayPrio.size()) {
      wayPrio.resize(type+1);
      wayMag.resize(type+1,magVeryClose);
      wayLineStyles.resize(type+1);
      wayRefLabelStyles.resize(type+1);
      wayNameLabelStyles.resize(type+1);
    }

    wayPrio[type]=prio;

    return *this;
  }

  StyleConfig& StyleConfig::SetWayMag(TypeId type, Mag mag)
  {
    if (type>=wayMag.size()) {
      wayPrio.resize(type+1,std::numeric_limits<size_t>::max());
      wayMag.resize(type+1);
      wayLineStyles.resize(type+1);
      wayRefLabelStyles.resize(type+1);
      wayNameLabelStyles.resize(type+1);
    }

    wayMag[type]=mag;

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaMag(TypeId type, Mag mag)
  {
    if (type>=areaMag.size()) {
      areaMag.resize(type+1);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    areaMag[type]=mag;

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeSymbolStyle(TypeId type,
                                               const SymbolStyle& style)
  {
    if (type>=nodeSymbolStyles.size()) {
      nodeSymbolStyles.resize(type+1,NULL);
      nodeRefLabelStyles.resize(type+1,NULL);
      nodeLabelStyles.resize(type+1,NULL);
      nodeIconStyles.resize(type+1,NULL);
    }

    delete nodeSymbolStyles[type];
    nodeSymbolStyles[type]=new SymbolStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeLabelStyle(TypeId type,
                                              const LabelStyle& style)
  {
    if (type>=nodeSymbolStyles.size()) {
      nodeSymbolStyles.resize(type+1,NULL);
      nodeRefLabelStyles.resize(type+1,NULL);
      nodeLabelStyles.resize(type+1,NULL);
      nodeIconStyles.resize(type+1,NULL);
    }

    delete nodeLabelStyles[type];
    nodeLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeRefLabelStyle(TypeId type,
                                                 const LabelStyle& style)
  {
    if (type>=nodeSymbolStyles.size()) {
      nodeSymbolStyles.resize(type+1,NULL);
      nodeRefLabelStyles.resize(type+1,NULL);
      nodeLabelStyles.resize(type+1,NULL);
      nodeIconStyles.resize(type+1,NULL);
    }

    delete nodeRefLabelStyles[type];
    nodeRefLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeIconStyle(TypeId type,
                                             const IconStyle& style)
  {
    if (type>=nodeSymbolStyles.size()) {
      nodeSymbolStyles.resize(type+1,NULL);
      nodeRefLabelStyles.resize(type+1,NULL);
      nodeLabelStyles.resize(type+1,NULL);
      nodeIconStyles.resize(type+1,NULL);
    }

    delete nodeIconStyles[type];
    nodeIconStyles[type]=new IconStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayLineStyle(TypeId type,
                                            const LineStyle& style)
  {
    if (type>=wayPrio.size()) {
      wayPrio.resize(type+1,std::numeric_limits<size_t>::max());
      wayMag.resize(type+1,magVeryClose);
      wayLineStyles.resize(type+1,NULL);
      wayRefLabelStyles.resize(type+1,NULL);
      wayNameLabelStyles.resize(type+1,NULL);
    }

    delete wayLineStyles[type];
    wayLineStyles[type]=new LineStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayRefLabelStyle(TypeId type,
                                                const LabelStyle& style)
  {
    if (type>=wayPrio.size()) {
      wayPrio.resize(type+1,std::numeric_limits<size_t>::max());
      wayMag.resize(type+1,magVeryClose);
      wayLineStyles.resize(type+1,NULL);
      wayRefLabelStyles.resize(type+1,NULL);
      wayNameLabelStyles.resize(type+1,NULL);
    }

    delete wayRefLabelStyles[type];
    wayRefLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayNameLabelStyle(TypeId type,
                                                 const LabelStyle& style)
  {
    if (type>=wayPrio.size()) {
      wayPrio.resize(type+1,std::numeric_limits<size_t>::max());
      wayMag.resize(type+1,magVeryClose);
      wayLineStyles.resize(type+1,NULL);
      wayRefLabelStyles.resize(type+1,NULL);
      wayNameLabelStyles.resize(type+1,NULL);
    }

    delete wayNameLabelStyles[type];
    wayNameLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaFillStyle(TypeId type,
                                             const FillStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaFillStyles[type];
    areaFillStyles[type]=new FillStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaBuildingFillStyle(TypeId type,
                                                     const FillStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaBuildingFillStyles[type];
    areaBuildingFillStyles[type]=new FillStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaPatternStyle(TypeId type,
                                                const PatternStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaPatternStyles[type];
    areaPatternStyles[type]=new PatternStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaLabelStyle(TypeId type,
                                              const LabelStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaLabelStyles[type];
    areaLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaSymbolStyle(TypeId type,
                                               const SymbolStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaSymbolStyles[type];
    areaSymbolStyles[type]=new SymbolStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaBorderStyle(TypeId type,
                                               const LineStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaBorderStyles[type];
    areaBorderStyles[type]=new LineStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaIconStyle(TypeId type,
                                             const IconStyle& style)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaBuildingFillStyles.resize(type+1,NULL);
      areaPatternStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaBorderStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }

    delete areaIconStyles[type];
    areaIconStyles[type]=new IconStyle(style);

    return *this;
  }

  size_t StyleConfig::GetStyleCount() const
  {
    size_t result=0;

    result=std::max(result,nodeSymbolStyles.size());
    result=std::max(result,wayLineStyles.size());
    result=std::max(result,areaFillStyles.size());
    result=std::max(result,areaBuildingFillStyles.size());
    result=std::max(result,areaPatternStyles.size());

    return result;
  }

  void StyleConfig::GetWayTypesByPrioWithMag(double mag,
                                             std::vector<TypeId>& types) const
  {
    types.clear();
    types.reserve(wayTypesByPrio.size());

    for (size_t i=0; i<wayTypesByPrio.size(); i++) {
      if (mag>=wayMag[wayTypesByPrio[i]]) {
        types.push_back(wayTypesByPrio[i]);
      }
    }
  }

  void StyleConfig::GetAreaTypesWithMag(double mag,
                                        TypeSet& types) const
  {
    types.Reset(areaMag.size());

    for (size_t i=0; i<areaMag.size(); i++) {
      if (mag>=areaMag[i]) {
        types.SetType(i);
      }
    }
  }

  void StyleConfig::GetNodeTypesWithMag(double mag,
                                        std::vector<TypeId>& types) const
  {
    types.clear();
    types.reserve(nodeSymbolStyles.size());

    for (size_t i=0; i<nodeSymbolStyles.size(); i++) {
      if (nodeLabelStyles[i]!=NULL &&
          mag>=nodeLabelStyles[i]->GetMinMag()) {
        types.push_back(i);
      }
      else if (nodeRefLabelStyles[i]!=NULL &&
          mag>=nodeRefLabelStyles[i]->GetMinMag()) {
        types.push_back(i);
      }
      else if (nodeSymbolStyles[i]!=NULL &&
               mag>=nodeSymbolStyles[i]->GetMinMag()) {
        types.push_back(i);
      }
      else if (nodeIconStyles[i]!=NULL &&
               mag>=nodeIconStyles[i]->GetMinMag()) {
        types.push_back(i);
      }
    }
  }

  /**
    Returns a sorted array (high priority with low numerical value to low priority with
    high numerical value) of used priorities.

    Example: [1,2,3,4,5,6,7,8,9,10,11,20]
    */
  void StyleConfig::GetPriorities(std::vector<size_t>& priorities) const
  {
    priorities=this->priorities;
  }
}

