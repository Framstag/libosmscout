#ifndef OSMSCOUT_GPX_EXTENSIONS_H
#define OSMSCOUT_GPX_EXTENSIONS_H

/*
  This source is part of the libosmscout-gpx library
  Copyright (C) 2017 Lukas Karas

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

#define EXTENTIONS_GPX_V3 "gpxx" /* http://www.garmin.com/xmlschemas/GpxExtensions/v3 */

namespace osmscout {
namespace gpx {

class OSMSCOUT_GPX_API Extension {
public:
  Extension() : xmlns(EXTENTIONS_GPX_V3) {}
  Extension(const char* ns, const std::string& name, const std::string& value)
    : xmlns(ns), name(name), value(value) {}

  const char* xmlns;
  std::string name;
  std::string value;

};

class OSMSCOUT_GPX_API Extensions {
public:
  Extensions() = default;

  std::vector<Extension> elements;

};

class OSMSCOUT_GPX_API TrackExtensions : public Extensions {
public:
  TrackExtensions() : Extensions(), xmlns(EXTENTIONS_GPX_V3) {}
  TrackExtensions(const char* ns) : Extensions(), xmlns(ns) {}

  const char* xmlns;

};
}
}

#endif //OSMSCOUT_GPX_EXTENSIONS_H
