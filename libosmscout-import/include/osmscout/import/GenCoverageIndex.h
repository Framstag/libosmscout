#ifndef OSMSCOUT_IMPORT_GENCOVERAGEINDEX_H
#define OSMSCOUT_IMPORT_GENCOVERAGEINDEX_H

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

#include <set>

#include <osmscout/util/Tiling.h>

#include <osmscout/import/Import.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class CoverageIndexGenerator CLASS_FINAL : public ImportModule
  {
  private:
    typedef std::set<Pixel> CellSet;

  private:
    TileCalculator tileCalculator;

  private:
    CellSet ScanNodes(const TypeConfigRef& typeConfig,
                      const ImportParameter& parameter,
                      Progress& progress) const;

    CellSet ScanWays(const TypeConfigRef& typeConfig,
                     const ImportParameter& parameter,
                     Progress& progress) const;

    CellSet ScanAreas(const TypeConfigRef& typeConfig,
                      const ImportParameter& parameter,
                      Progress& progress) const;

    void WriteIndex(const ImportParameter& parameter,
                    Progress& progress,
                    const CellSet& cells);
  public:
    CoverageIndexGenerator();
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
