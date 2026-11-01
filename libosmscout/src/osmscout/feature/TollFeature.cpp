/*
This source is part of the libosmscout library
Copyright (C) 2026  Tim Teulings

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

#include <osmscout/feature/TollFeature.h>

namespace osmscout {

  void TollFeatureValue::Read(FileScanner& scanner)
  {
    toll=scanner.ReadUInt8();
  }

  void TollFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(toll);
  }

  TollFeatureValue& TollFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const TollFeatureValue&>(other);

      toll=otherValue.toll;
    }

    return *this;
  }

  bool TollFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const TollFeatureValue&>(other);

    return toll==otherValue.toll;
  }

  const char* const TollFeature::NAME = "Toll";

  void TollFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagToll=tagRegistry.RegisterTag("toll");
  }

  std::string TollFeature::GetName() const
  {
    return NAME;
  }

  size_t TollFeature::GetValueAlignment() const
  {
    return alignof(TollFeatureValue);
  }

  size_t TollFeature::GetValueSize() const
  {
    return sizeof(TollFeatureValue);
  }

  FeatureValue* TollFeature::AllocateValue(void* buffer)
  {
    return new (buffer) TollFeatureValue();
  }

  void TollFeature::Parse(TagErrorReporter& /*errorReporter*/,
                          const TagRegistry& /*tagRegistry*/,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
                          FeatureValueBuffer& buffer) const
  {
    auto toll=tags.find(tagToll);

    if (toll==tags.end()) {
      return;
    }

    // OSM uses "yes"/"no" for a charging way, and the numeric forms and "true"/"false" appear in the
    // wild. Everything else is treated as "no toll": an unparsed value must not invent a price.
    bool charges=toll->second=="yes" || toll->second=="true" || toll->second=="1";

    if (!charges) {
      return;
    }

    auto* value=static_cast<TollFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

    value->SetToll(1);
  }
}
