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

#include <cctype>
#include <cstring>
#include <limits>
#include <sstream>

#include <osmscout/util/String.h>

namespace osmscout {

  namespace {
    /**
     * Unit definition for distance value parsing.
     * Add new entries here to support additional units.
     */
    struct DistanceUnit {
      const char* suffix;
      double factorToMeters;
    };

    static constexpr DistanceUnit distanceUnits[] = {
      {"km", 1000.0},
      {"mi", 1609.344}
    };

    /**
     * Check if string ends with given suffix, ensuring suffix is
     * preceded by non-alpha character (to avoid e.g. "nmi" matching "mi").
     */
    bool EndsWithUnitSuffix(const std::string& str, const char* suffix)
    {
      size_t suffixLen = std::strlen(suffix);
      if (str.size() < suffixLen) {
        return false;
      }
      if (str.compare(str.size() - suffixLen, suffixLen, suffix) != 0) {
        return false;
      }
      size_t pos = str.size() - suffixLen;
      // Suffix must be preceded by start of string or non-alpha character
      return pos == 0 || !std::isalpha(static_cast<unsigned char>(str[pos - 1]));
    }

    /**
     * Try to parse distance string with a known unit suffix.
     * Returns true if parsed and value was set.
     */
    bool ParseWithUnit(const std::string& distanceString,
                       const TagMap& tags,
                       TagErrorReporter& errorReporter,
                       const ObjectOSMRef& object,
                       HighwayMilestoneFeatureValue* value)
    {
      for (const auto& unit : distanceUnits) {
        if (!EndsWithUnitSuffix(distanceString, unit.suffix)) {
          continue;
        }
        size_t suffixLen = std::strlen(unit.suffix);
        std::string prefix = distanceString.substr(0, distanceString.size() - suffixLen);
        prefix = Trim(prefix);

        double d;
        if (!StringToNumber(prefix, d)) {
          errorReporter.ReportTag(object, tags,
                                  std::string("HighwayMilestone distance tag value '") +
                                  distanceString + "' is not a valid number!");
          return true;
        }
        if (d < 0 || d > (std::numeric_limits<uint32_t>::max() / unit.factorToMeters)) {
          errorReporter.ReportTag(object, tags,
                                  std::string("HighwayMilestone distance tag value '") +
                                  distanceString + "' is out of range!");
          return true;
        }
        value->SetDistance(static_cast<uint32_t>(d * unit.factorToMeters));
        return true;
      }
      return false;
    }
  }

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
    auto* value=static_cast<HighwayMilestoneFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

    auto distanceTag=tags.find(tagDistance);
    if (distanceTag!=tags.end()) {
      std::string distanceString=Trim(distanceTag->second);
      double      d;

      // Try parsing with known unit suffix first
      if (ParseWithUnit(distanceString, tags, errorReporter, object, value)) {
        // Distance was set (or error reported), nothing more to do
      } else if (!StringToNumber(distanceString,d)) {
        errorReporter.ReportTag(object,tags,std::string("HighwayMilestone distance tag value '")+distanceTag->second+"' is not a valid number!");
      } else if (d<0 || d>(std::numeric_limits<uint32_t>::max()/1000.0)) {
        errorReporter.ReportTag(object,tags,std::string("HighwayMilestone distance tag value '")+distanceTag->second+"' is out of range!");
      } else {
        value->SetDistance(static_cast<uint32_t>(d * 1000.0));
      }
    }

    auto refTag=tags.find(tagRef);
    if (refTag!=tags.end()) {
      value->SetRef(refTag->second);
    }

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
