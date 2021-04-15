#ifndef OSMSCOUT_GPX_EXTENSIONS_H
#define OSMSCOUT_GPX_EXTENSIONS_H

/*
  This source is part of the libosmscout-gpx library
  Copyright (C) 2021 Lukas Karas

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

#include <osmscout/gpx/GPXImportExport.h>

#include <string>
#include <vector>

namespace osmscout {
namespace gpx {

class OSMSCOUT_GPX_API Extensions {

public:
  Extensions() = default;

  class Element {
  public:
    explicit Element(const std::string& ns, const std::string& elemName, const std::string& elemValue);

    const std::string& GetName() const { return name; }
    const std::string& GetValue() const { return value; }

  private:
    std::string name;
    std::string value;

  };

  std::vector<Element> elements;

};
}
}

#endif //OSMSCOUT_GPX_EXTENSIONS_H
