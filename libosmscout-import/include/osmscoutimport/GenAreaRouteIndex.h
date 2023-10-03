#ifndef OSMSCOUT_IMPORT_GENAREAROUTEINDEX_H
#define OSMSCOUT_IMPORT_GENAREAROUTEINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Lukas Karas

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

#include <osmscoutimport/Import.h>
#include <osmscoutimport/AreaIndexGenerator.h>

#include <list>
#include <map>

#include <osmscout/Pixel.h>
#include <osmscout/Route.h>

#include <osmscout/io/FileWriter.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/TileId.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class AreaRouteIndexGenerator CLASS_FINAL : public AreaIndexGenerator<Route>
  {
  private:
    void WriteTypeId(const TypeConfigRef& typeConfig,
                     const TypeInfoRef &type,
                     FileWriter &writer) const override;

  public:
    AreaRouteIndexGenerator();
    ~AreaRouteIndexGenerator() override = default;

    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif