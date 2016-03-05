#ifndef OSMSCOUT_IMPORT_PREPROCESS_H
#define OSMSCOUT_IMPORT_PREPROCESS_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <unordered_map>

#include <osmscout/Coord.h>
#include <osmscout/Tag.h>
#include <osmscout/TurnRestriction.h>

#include <osmscout/util/FileWriter.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/Preprocessor.h>
#include <osmscout/import/RawRelation.h>


namespace osmscout {
  class Preprocess : public ImportModule
  {
  public:
    static const char* BOUNDING_DAT;
    static const char* DISTRIBUTION_DAT;
    static const char* RAWNODES_DAT;
    static const char* RAWWAYS_DAT;
    static const char* RAWRELS_DAT;
    static const char* RAWCOASTLINE_DAT;
    static const char* RAWTURNRESTR_DAT;

  private:
    class Callback : public PreprocessorCallback
    {
    private:
      struct CoordPage
      {
        FileOffset             offset;
        PageId                 id;
        std::vector<GeoCoord>  coords;
        std::vector<bool>      isSet;

        CoordPage(size_t coordPageSize,FileOffset offset,PageId id);

        void SetCoord(size_t index, const GeoCoord& coord);

        void StorePage(FileWriter& writer);
        void ReadPage(FileScanner& scanner);
      };

      //! Reference to a page
      typedef std::shared_ptr<CoordPage>            CoordPageRef;

      //! Teh cache is just a list of pages
      typedef std::list<CoordPageRef>               CoordPageCache;

      //! References to a page in above list
      typedef CoordPageCache::iterator              CoordPageCacheRef;

      //! An index from PageIds to cache entries
      typedef std::map<PageId,CoordPageCacheRef>    CoordPageCacheIndex;

      //! Index off all PageIds to their FileOffsets
      typedef std::unordered_map<PageId,FileOffset> CoordPageOffsetMap;

    private:
      const TypeConfigRef    typeConfig;
      const ImportParameter& parameter;
      Progress&              progress;

      FileWriter             nodeWriter;
      FileWriter             wayWriter;
      FileWriter             coastlineWriter;
      FileWriter             turnRestrictionWriter;
      FileWriter             multipolygonWriter;

      uint32_t               coordCount;
      uint32_t               nodeCount;
      uint32_t               wayCount;
      uint32_t               areaCount;
      uint32_t               relationCount;
      uint32_t               coastlineCount;
      uint32_t               turnRestrictionCount;
      uint32_t               multipolygonCount;

      OSMId                  lastNodeId;
      OSMId                  lastWayId;
      OSMId                  lastRelationId;

      bool                   nodeSortingError;
      bool                   waySortingError;
      bool                   relationSortingError;

      Id                     coordPageCount;
      CoordPageCacheIndex    coordPageIndex;
      CoordPageCache         coordPageCache;
      CoordPageOffsetMap     coordPageOffsetMap;
      FileScanner            coordScanner;
      FileWriter             coordWriter;
      CoordPageRef currentPage;

      GeoCoord               minCoord;
      GeoCoord               maxCoord;

      std::vector<uint32_t>  nodeStat;
      std::vector<uint32_t>  areaStat;
      std::vector<uint32_t>  wayStat;

    private:
      CoordPageRef GetCoordPage(PageId id);
      void StoreCoord(OSMId id,
                      const GeoCoord& coord);

      bool IsTurnRestriction(const TagMap& tags,
                             TurnRestriction::Type& type) const;

      void ProcessTurnRestriction(const std::vector<RawRelation::Member>& members,
                                  TurnRestriction::Type type);

      bool IsMultipolygon(const TagMap& tags,
                          TypeInfoRef& type);

      void ProcessMultipolygon(const TagMap& tags,
                               const std::vector<RawRelation::Member>& members,
                               OSMId id,
                               const TypeInfoRef& type);

      bool DumpDistribution();
      bool DumpBoundingBox();

    public:
      Callback(const TypeConfigRef& typeConfig,
               const ImportParameter& parameter,
               Progress& progress);

      bool Initialize();

      bool Cleanup(bool success);

      void ProcessNode(const OSMId& id,
                       const double& lon, const double& lat,
                       const TagMap& tags);
      void ProcessWay(const OSMId& id,
                      std::vector<OSMId>& nodes,
                      const TagMap& tags);
      void ProcessRelation(const OSMId& id,
                           const std::vector<RawRelation::Member>& members,
                           const TagMap& tags);
    };

  private:
    bool ProcessFiles(const TypeConfigRef& typeConfig,
                      const ImportParameter& parameter,
                      Progress& progress,
                      Callback& callback);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
