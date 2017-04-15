/*
  DumpOSS - a demo program for libosmscout
  Copyright (C) 2015  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstdlib>

#include <osmscout/TypeConfig.h>
#include <osmscout/StyleConfig.h>

void DumpFillStyleAttributes(const std::set<osmscout::FillStyle::Attribute> attributes,
                             const osmscout::FillStyleRef& style)
{
  std::cout << "{";

  for (const auto& attribute : attributes) {
    switch (attribute) {
    case osmscout::FillStyle::attrFillColor:
      std::cout << "color: " << "#" << style->GetFillColor().ToHexString() << ";";
      break;
    }
  }

  std::cout << "}";
}

void DumpTextStyleAttributes(const std::set<osmscout::TextStyle::Attribute> attributes,
                             const osmscout::TextStyleRef& style)
{
  std::cout << "{";

  for (const auto& attribute : attributes) {
    switch (attribute) {
    case osmscout::TextStyle::attrLabel:
      std::cout << "label: " << style->GetLabel()->GetName() << ";";
      break;
    }
  }

  std::cout << "}";
}

void DumpType(const osmscout::TypeConfigRef& /*typeConfig*/,
              const osmscout::StyleConfigRef& styleConfig,
              size_t level,
              const osmscout::TypeInfoRef& type)
{
  // Node
  std::list<osmscout::TextStyleSelector> nodeTextStyleSelectors;

  // Area
  std::list<osmscout::FillStyleSelector> areaFillStyleSelectors;
  std::list<osmscout::TextStyleSelector> areaTextStyleSelectors;

  if (type->CanBeNode()) {
    styleConfig->GetNodeTextStyleSelectors(level,
                                           type,
                                           nodeTextStyleSelectors);
  }

  if (type->CanBeArea()) {
    styleConfig->GetAreaFillStyleSelectors(level,
                                           type,
                                           areaFillStyleSelectors);

    styleConfig->GetAreaTextStyleSelectors(level,
                                           type,
                                           areaTextStyleSelectors);
  }

  if (!nodeTextStyleSelectors.empty() ||
      !areaFillStyleSelectors.empty() ||
      !areaTextStyleSelectors.empty()) {
    std::cout << "    [TYPE " << type->GetName() <<"] {" << std::endl;

    for (const auto& selector : areaFillStyleSelectors) {
      if (selector.criteria.HasCriteria()) {
        std::cout << "      [";
        if (selector.criteria.GetOneway()) {
          std::cout << " ONEWAY";
        }
        std::cout << "] {" << std::endl;
      }

      std::cout << "      AREA ";

      DumpFillStyleAttributes(selector.attributes,
                              selector.style);

      std::cout << std::endl;

      if (selector.criteria.HasCriteria()) {
        std::cout << "}" << std::endl;
      }
    }

    for (const auto& selector : areaTextStyleSelectors) {
      if (selector.criteria.HasCriteria()) {
        std::cout << "      [";
        if (selector.criteria.GetOneway()) {
          std::cout << " ONEWAY";
        }
        std::cout << "] {" << std::endl;
      }

      std::cout << "      AREA.TEXT";

      if (!selector.style->GetSlot().empty()) {
        std::cout << "#" << selector.style->GetSlot();
      }

      std::cout << " ";

      DumpTextStyleAttributes(selector.attributes,
                              selector.style);

      std::cout << std::endl;

      if (selector.criteria.HasCriteria()) {
        std::cout << "}" << std::endl;
      }
    }

    for (const auto& selector : nodeTextStyleSelectors) {
      if (selector.criteria.HasCriteria()) {
        std::cout << "      [";
        if (selector.criteria.GetOneway()) {
          std::cout << " ONEWAY";
        }
        std::cout << "] {" << std::endl;
      }

      std::cout << "      NODE.TEXT";

      if (!selector.style->GetSlot().empty()) {
        std::cout << "#" << selector.style->GetSlot();
      }

      std::cout << " ";

      DumpTextStyleAttributes(selector.attributes,
                              selector.style);

      std::cout << std::endl;

      if (selector.criteria.HasCriteria()) {
        std::cout << "}" << std::endl;
      }
    }

    std::cout << "    }" << std::endl;
  }
}

void DumpLevel(const osmscout::TypeConfigRef& typeConfig,
               const osmscout::StyleConfigRef& styleConfig,
               size_t level)
{
  osmscout::MagnificationConverter magConverter;
  std::string                      magName;

  if (level>0) {
    std::cout << std::endl;
  }

  std::cout << "  [MAG ";

  if (magConverter.Convert(level,
                           magName)) {
    std::cout << magName << " /* " << level << " */";
  }
  else {
    std::cout << level;
  }

  std::cout << "] {" << std::endl;

  for (const auto& type : typeConfig->GetTypes()) {
    DumpType(typeConfig,
             styleConfig,
             level,
             type);
  }

  std::cout << "  }" << std::endl;
}

void DumpOSSFile(const osmscout::TypeConfigRef& typeConfig,
                 const osmscout::StyleConfigRef& styleConfig)
{
  std::cout << "OSS" << std::endl;

  for (size_t level=0; level<=20; level++) {
    DumpLevel(typeConfig,
              styleConfig,
              level);

  }

  std::cout << "END" << std::endl;
}

int main(int argc, char* argv[])
{
  std::string                      ostFile;
  std::string                      ossFile;

  if (argc!=3) {
    std::cerr << "DumpOSS <OST file> <OSS file>" << std::endl;
    return 1;
  }

  ostFile=argv[1];
  ossFile=argv[2];

  osmscout::TypeConfigRef typeConfig(new osmscout::TypeConfig());

  if (!typeConfig->LoadFromOSTFile(ostFile)) {
    std::cerr << "Cannot load OST file '" << ostFile << "'" << std::endl;
    return 1;
  }

  osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(typeConfig));

  if (!styleConfig->Load(ossFile)) {
    std::cerr << "Cannot load OSS file '" << ostFile << "'" << std::endl;
    return 1;
  }

  DumpOSSFile(typeConfig,
              styleConfig);

  styleConfig=NULL;
  typeConfig=NULL;

  return 0;
}
