#ifndef OSMSCOUT_IMPORT_GENRELAREADAT_H
#define OSMSCOUT_IMPORT_GENRELAREADAT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <map>
#include <unordered_map>
#include <unordered_set>

#include <osmscout/Area.h>

#include <osmscout/DataFile.h>
#include <osmscout/CoordDataFile.h>

#include <osmscout/util/Geometry.h>

#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawRelIndexedDataFile.h>
#include <osmscout/import/RawWay.h>
#include <osmscout/import/RawWayIndexedDataFile.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class RelAreaDataGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* RELAREA_TMP;
    static const char* WAYAREABLACK_DAT;

  private:
    typedef std::unordered_set<OSMId>           IdSet;

    typedef std::unordered_map<OSMId,RawWayRef> IdRawWayMap;

  private:
    class GroupingState
    {
    private:
      size_t rings;
      bool   *used;
      bool   *includes;
      bool   *hasIncludes;

    public:
      GroupingState(size_t rings)
      {
        this->rings=rings;

        used=new bool[rings];
        for (size_t i=0; i<rings; i++) {
          used[i]=false;
        }

        includes=new bool[rings*rings];
        for (size_t i=0; i<rings*rings; i++) {
          includes[i]=false;
        }

        hasIncludes=new bool[rings];
        for (size_t i=0; i<rings; i++) {
          hasIncludes[i]=false;
        }
      }

      ~GroupingState()
      {
        delete [] hasIncludes;
        delete [] includes;
        delete [] used;
      }

      inline size_t GetRingCount() const
      {
        return rings;
      }

      inline void SetUsed(size_t used)
      {
        this->used[used]=true;
      }

      inline bool IsUsed(size_t used) const
      {
        return this->used[used];
      }

      inline void SetIncluded(size_t includer, size_t included)
      {
        hasIncludes[includer]=true;
        includes[included*rings+includer]=true;
      }

      inline bool HasIncludes(size_t includer) const
      {
        return hasIncludes[includer];
      }

      inline bool Includes(size_t included, size_t includer) const
      {
        return includes[included*rings+includer];
      }
    };

    struct MultipolygonPart
    {
      Area::Ring           role;
      std::list<RawWayRef> ways;

      inline bool IsArea() const
      {
        if (ways.size()==1) {
          return ways.front()->IsArea() ||
                 ways.front()->GetNodes().front()==ways.front()->GetNodes().back();
        }
        else {
          return false;
        }
      }
    };

  private:
    std::list<MultipolygonPart>::const_iterator FindTopLevel(const std::list<MultipolygonPart>& rings,
                                                             const GroupingState& state,
                                                             size_t& topIndex);

    std::list<MultipolygonPart>::const_iterator FindSub(const std::list<MultipolygonPart>& rings,
                                                        size_t topIndex,
                                                        const GroupingState& state,
                                                        size_t& subIndex);

    void ConsumeSubs(const std::list<MultipolygonPart>& rings,
                     std::list<MultipolygonPart>& groups,
                     GroupingState& state,
                     size_t topIndex,
                     size_t id);

    bool BuildRings(const TypeConfig& typeConfig,
                    const ImportParameter& parameter,
                    Progress& progress,
                    Id id,
                    const std::string& name,
                    std::list<MultipolygonPart>& parts);

    bool ResolveMultipolygon(const TypeConfig& typeConfig,
                             const ImportParameter& parameter,
                             Progress& progress,
                             Id id,
                             const std::string& name,
                             std::list<MultipolygonPart>& parts);

    bool ComposeAreaMembers(const TypeConfig& typeConfig,
                            Progress& progress,
                            const CoordDataFile::ResultMap& coordMap,
                            const IdRawWayMap& wayMap,
                            const std::string& name,
                            const RawRelation& rawRelation,
                            std::list<MultipolygonPart>& parts);

    bool ComposeBoundaryMembers(const TypeConfig& typeConfig,
                                Progress& progress,
                                const CoordDataFile::ResultMap& coordMap,
                                const IdRawWayMap& wayMap,
                                const std::map<OSMId,RawRelationRef>& relationMap,
                                const Area& relation,
                                const std::string& name,
                                const RawRelation& rawRelation,
                                IdSet& resolvedRelations,
                                std::list<MultipolygonPart>& parts);

  bool ResolveMultipolygonMembers(Progress& progress,
                                  const TypeConfig& typeConfig,
                                  CoordDataFile& coordDataFile,
                                  RawWayIndexedDataFile& wayDataFile,
                                  RawRelationIndexedDataFile& relDataFile,
                                  IdSet& resolvedRelations,
                                  const Area& relation,
                                  const std::string& name,
                                  const RawRelation& rawRelation,
                                  std::list<MultipolygonPart>& parts);

    bool HandleMultipolygonRelation(const ImportParameter& parameter,
                                    Progress& progress,
                                    const TypeConfig& typeConfig,
                                    IdSet& wayAreaIndexBlacklist,
                                    CoordDataFile& coordDataFile,
                                    RawWayIndexedDataFile& wayDataFile,
                                    RawRelationIndexedDataFile& relDataFile,
                                    RawRelation& rawRelation,
                                    const std::string& name,
                                    Area& relation);

    std::string ResolveRelationName(const FeatureRef& featureName,
                                    const RawRelation& rawRelation) const;

    TypeInfoRef AutodetectRelationType(const ImportParameter& parameter,
                                       const TypeConfig& typeConfig,
                                       const RawRelation& rawRelation,
                                       std::list<MultipolygonPart>& parts,
                                       std::list<MultipolygonPart>::iterator& copyPart) const;

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
