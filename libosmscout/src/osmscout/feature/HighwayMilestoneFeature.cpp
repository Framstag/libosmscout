/*
  This source is part of the libosmscout library
  Copyright (C) 2025  Tim Teulings

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

#include <osmscout/feature/HighwayMilestoneFeature.h>

#include <sstream>

#include <osmscout/util/String.h>

namespace osmscout {

  void HighwayMilestoneFeatureValue::Read(FileScanner& scanner)
  {
    distance=scanner.ReadUInt32Number();
    ref=scanner.ReadString();
    carriagewayRef=scanner.ReadString();
    marker=scanner.ReadString();
  }

  void HighwayMilestoneFeatureValue::Write(FileWriter& writer)
  {
    writer.WriteNumber(distance);
    writer.Write(ref);
    writer.Write(carriagewayRef);
    writer.Write(marker);
  }

  HighwayMilestoneFeatureValue& HighwayMilestoneFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const HighwayMilestoneFeatureValue&>(other);

      distance=otherValue.distance;
      ref=otherValue.ref;
      carriagewayRef=otherValue.carriagewayRef;
      marker=otherValue.marker;
    }

    return *this;
  }

  bool HighwayMilestoneFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const HighwayMilestoneFeatureValue&>(other);

    return distance==otherValue.distance &&
           ref==otherValue.ref &&
           carriagewayRef==otherValue.carriagewayRef &&
           marker==otherValue.marker;
  }

  std::string HighwayMilestoneFeatureValue::GetLabel(const Locale &locale, size_t /*labelIndex*/) const
  {
    std::stringstream ss;
    ss << NumberToString(distance / 1000, locale);
    ss << locale.GetUnitsSeparator();
    ss << "km";
    return ss.str();
  }

  const char* const HighwayMilestoneFeature::NAME = "HighwayMilestone";

  void HighwayMilestoneFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagDistance=tagRegistry.RegisterTag("distance");
    tagRef=tagRegistry.RegisterTag("ref");
    tagCarriagewayRef=tagRegistry.RegisterTag("carriageway_ref");
    tagMarker=tagRegistry.RegisterTag("marker");
  }

  std::string HighwayMilestoneFeature::GetName() const
  {
    return NAME;
  }

  size_t HighwayMilestoneFeature::GetValueAlignment() const
  {
    return alignof(HighwayMilestoneFeatureValue);
  }

  size_t HighwayMilestoneFeature::GetValueSize() const
  {
    return sizeof(HighwayMilestoneFeatureValue);
  }

  FeatureValue* HighwayMilestoneFeature::AllocateValue(void* buffer)
  {
    return new (buffer) HighwayMilestoneFeatureValue();
  }

  void HighwayMilestoneFeature::Parse(TagErrorReporter& errorReporter,
                                      const TagRegistry& /*tagRegistry*/,
                                      const FeatureInstance& feature,
                                      const ObjectOSMRef& object,
                                      const TagMap& tags,
                                      FeatureValueBuffer& buffer) const
  {
    auto distanceTag=tags.find(tagDistance);
    auto refTag=tags.find(tagRef);

    if (distanceTag==tags.end() || refTag==tags.end()) {
      return;
    }

    std::string distanceString=distanceTag->second;
    double      d;

    if (!StringToNumber(distanceString,d)) {
      errorReporter.ReportTag(object,tags,std::string("HighwayMilestone distance tag value '")+distanceTag->second+"' is not a valid number!");
      return;
    }

    // OSM distance is in km; multiply by 1000 for internal meter storage. Range check prevents overflow after multiplication.
    if (d<0 || d>(std::numeric_limits<uint32_t>::max()/1000.0)) {
      errorReporter.ReportTag(object,tags,std::string("HighwayMilestone distance tag value '")+distanceTag->second+"' is out of range!");
      return;
    }

    auto* value=static_cast<HighwayMilestoneFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

    value->SetDistance(static_cast<uint32_t>(d * 1000.0));
    value->SetRef(refTag->second);

    auto carriagewayRefTag=tags.find(tagCarriagewayRef);
    if (carriagewayRefTag!=tags.end()) {
      value->SetCarriagewayRef(carriagewayRefTag->second);
    }

    auto markerTag=tags.find(tagMarker);
    if (markerTag!=tags.end()) {
      value->SetMarker(markerTag->second);
    }
  }
}
