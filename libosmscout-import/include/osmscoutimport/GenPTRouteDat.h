#ifndef OSMSCOUT_IMPORT_GENPTROUTEDAT_H
#define OSMSCOUT_IMPORT_GENPTROUTEDAT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Tim Teulings

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

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/PublicTransport.h>

#include <osmscoutimport/Import.h>

namespace osmscout {

  class PTRouteDataGenerator CLASS_FINAL : public ImportModule
  {
  private:
    static bool WriteRoutes(const TypeConfig& typeConfig,
                     const ImportParameter& parameter,
                     Progress& progress,
                     const std::list<PTRouteRef>& routes);

  public:
    PTRouteDataGenerator() = default;

    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
