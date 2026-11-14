#ifndef OSMSCOUT_FEATURE_TOLL_FEATURE_H
#define OSMSCOUT_FEATURE_TOLL_FEATURE_H

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

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

namespace osmscout {

  /**
   * Whether using an object costs a toll.
   *
   * The feature exists so that a routing profile can see the toll during the search: the search prices
   * an object variant, not a way, and `ObjectVariantData` carries only the type, the maximum speed and
   * the grade. A database whose type configuration has no Toll feature has no toll information, and
   * every object of it reads as "no toll" — absence of the signal is never a statement about price.
   */
  class OSMSCOUT_API TollFeatureValue : public FeatureValue
  {
  private:
    uint8_t toll=0; //!< 0: no toll, 1: using the object costs a toll

  public:
    TollFeatureValue() = default;

    explicit TollFeatureValue(uint8_t toll)
      : toll(toll)
    {
      // no code
    }

    void SetToll(uint8_t toll)
    {
      this->toll=toll;
    }

    bool IsToll() const
    {
      return toll!=0;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    TollFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API TollFeature : public Feature
  {
  private:
    TagId tagToll;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueAlignment() const override;
    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  using TollFeatureValueReader = FeatureValueReader<TollFeature, TollFeatureValue>;
}

#endif
