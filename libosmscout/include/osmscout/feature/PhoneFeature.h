#ifndef OSMSCOUT_FEATURE_PHONE_FEATURE_H
#define OSMSCOUT_FEATURE_PHONE_FEATURE_H

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

  class OSMSCOUT_API PhoneFeatureValue : public FeatureValue
  {
  private:
    std::string phone;

  public:
    PhoneFeatureValue() = default;

    explicit PhoneFeatureValue(const std::string& phone)
    : phone(phone)
    {
      // no code
    }

    void SetPhone(const std::string_view& phone)
    {
      this->phone=phone;
    }

    std::string GetPhone() const
    {
      return phone;
    }

    std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return phone;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    PhoneFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API PhoneFeature : public Feature
  {
  private:
    TagId tagPhone;
    TagId tagContactPhone;
    TagId tagContactMobile;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "number" label */
    static const char* const NUMBER_LABEL;

    /** Index of the 'number' label */
    static const size_t      NUMBER_LABEL_INDEX;

  public:
    PhoneFeature();
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
}

#endif
