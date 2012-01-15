#ifndef OSMSCOUT_IMPORT_GENOPTIMIZELOWZOOM_H
#define OSMSCOUT_IMPORT_GENOPTIMIZELOWZOOM_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/import/Import.h>

#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

namespace osmscout {

  class OptimizeLowZoomGenerator : public ImportModule
  {
  private:
    void GetTypesToOptimize(const TypeConfig& typeConfig,
                            std::set<TypeId>& types);

    void WriteHeader(FileWriter& writer,
                     const std::set<TypeId>& types,
                     size_t optimizeMaxMap,
                     std::map<TypeId,FileOffset>& typeOffsetMap);

    bool GetWaysToOptimize(Progress& progress,
                           FileScanner& scanner,
                           TypeId type,
                           std::list<WayRef>& ways);

    void MergeWays(const std::list<WayRef>& ways,
                   std::list<WayRef>& newWays);

    bool OptimizeWriteWays(Progress& progress,
                           FileWriter& writer,
                           FileOffset typeFileOffset,
                           const std::list<WayRef>& newWays,
                           size_t width,
                           size_t height,
                           double magnification);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
