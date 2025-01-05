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

#include <osmscout/feature/WebsiteFeature.h>

namespace osmscout {

  void WebsiteFeatureValue::Read(FileScanner& scanner)
  {
    website=scanner.ReadString();
  }

  void WebsiteFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(website);
  }

  WebsiteFeatureValue& WebsiteFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const WebsiteFeatureValue&>(other);

      website=otherValue.website;
    }

    return *this;
  }

  bool WebsiteFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const WebsiteFeatureValue&>(other);

    return website==otherValue.website;
  }

  const char* const WebsiteFeature::NAME = "Website";
  const char* const WebsiteFeature::URL_LABEL = "url";
  const size_t      WebsiteFeature::URL_LABEL_INDEX = 0;

  WebsiteFeature::WebsiteFeature()
  {
    RegisterLabel(URL_LABEL_INDEX,URL_LABEL);
  }

  void WebsiteFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagWebsite=tagRegistry.RegisterTag("website");
    tagContactWebsite=tagRegistry.RegisterTag("contact:website");
  }

  std::string WebsiteFeature::GetName() const
  {
    return NAME;
  }

  size_t WebsiteFeature::GetValueAlignment() const
  {
    return alignof(WebsiteFeatureValue);
  }

  size_t WebsiteFeature::GetValueSize() const
  {
    return sizeof(WebsiteFeatureValue);
  }

  FeatureValue* WebsiteFeature::AllocateValue(void* buffer)
  {
    return new (buffer) WebsiteFeatureValue();
  }

  void WebsiteFeature::Parse(TagErrorReporter& errorReporter,
                             const TagRegistry& /*tagRegistry*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& object,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    std::string strValue;

    std::vector<TagId> websiteTags{tagWebsite, tagContactWebsite};
    for (auto tagId : websiteTags) {
      auto website = tags.find(tagId);
      if (website != tags.end()) {
        strValue = website->second;
        break; // use the first one
      }
    }

    try {
      if (!strValue.empty()) {
        size_t idx = feature.GetIndex();
        FeatureValue* fv = buffer.AllocateValue(idx);
        auto* value=static_cast<WebsiteFeatureValue*>(fv);

        value->SetWebsite(strValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Website parse exception: ")+e.what());
    }
  }
}
