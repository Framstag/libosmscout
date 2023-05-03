#ifndef OSMSCOUT_FEATURE_CONSTRUCTION_YEAR_FEATURE_H
#define OSMSCOUT_FEATURE_CONSTRUCTION_YEAR_FEATURE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

  class OSMSCOUT_API ConstructionYearFeatureValue : public FeatureValue
  {
  private:
    int startYear=0;
    int endYear=0;

  public:
    ConstructionYearFeatureValue() = default;

    ConstructionYearFeatureValue(int startYear, int endYear)
      : startYear(startYear),
        endYear(endYear)
    {
      // no code
    }

    void SetStartYear(int year)
    {
      this->startYear=year;
    }

    int GetStartYear() const
    {
      return startYear;
    }

    void SetEndYear(int year)
    {
      this->endYear=year;
    }

    int GetEndYear() const
    {
      return endYear;
    }

    std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      if (startYear==endYear) {
        return std::to_string(startYear);
      }

      return std::to_string(startYear)+"-"+std::to_string(endYear);
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    ConstructionYearFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API ConstructionYearFeature : public Feature
  {
  private:
    TagId tagConstructionYear;
    TagId tagStartDate;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "year" label */
    static const char* const YEAR_LABEL;

    /** Index of the 'year' label */
    static const size_t      YEAR_LABEL_INDEX;

  public:
    ConstructionYearFeature();
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

  using ConstructionYearFeatureValueReader = FeatureValueReader<ConstructionYearFeature, ConstructionYearFeatureValue>;
}

#endif
