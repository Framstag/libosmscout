  /*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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

#include <osmscout/TypeFeature.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  FeatureValue& FeatureValue::operator=(const FeatureValue& /*other*/) // NOLINT
  {
    assert(false);

    return *this;
  }

  void Feature::RegisterLabel(size_t index,
                              const std::string& labelName)
  {
    assert(!labels.contains(labelName));

    labels[labelName]=index;
  }

  void Feature::RegisterFlag(size_t index,
                             const std::string& flagName)
  {
    assert(!flags.contains(flagName));

    flags[flagName]=index;
  }

  bool Feature::GetLabelIndex(const std::string& labelName,
                              size_t& index) const
  {
    const auto entry=labels.find(labelName);

    if (entry==labels.end()) {
      return false;
    }

    index=entry->second;

    return true;
  }

  bool Feature::GetFlagIndex(const std::string& flagName,
                             size_t& index) const
  {
    const auto entry=flags.find(flagName);

    if (entry==flags.end()) {
      return false;
    }

    index=entry->second;

    return true;
  }

  FeatureValue* Feature::AllocateValue(void* /*buffer*/)
  {
    assert(false);
    return nullptr;
  }

  /**
   * Add a description of the feature for the given language code
   * @param languageCode
   *    language code like for example 'en'or 'de'
   * @param description
   *    description of the type
   * @return
   *    type info instance
   */
  void Feature::AddDescription(const std::string& languageCode,
                               const std::string& description)
  {
    descriptions[languageCode]=description;
  }

  /**
   * Returns the description for the given language code. Returns an empty string, if
   * no description is available for the given language code.
   *
   * @param languageCode
   *    languageCode like for example 'en' or 'de'
   * @return
   *    Description or empty string
   */
  std::string Feature::GetDescription(const std::string& languageCode) const
  {
    auto entry=descriptions.find(languageCode);

    if (entry!=descriptions.end()) {
      return entry->second;
    }

    return "";
  }

  /**
   * Just to make the compiler happy :-/
   */
  FeatureInstance::FeatureInstance()
    : type(nullptr),
      featureBit(0),
      index(0),
      offset(0)
  {

  }

  FeatureInstance::FeatureInstance(const FeatureRef& feature,
                                   const TypeInfo* type,
                                   size_t featureBit,
                                   size_t index,
                                   size_t offset)
    : feature(feature),
      type(type),
      featureBit(featureBit),
      index(index),
      offset(offset)
  {
    assert(feature);
  }
}
