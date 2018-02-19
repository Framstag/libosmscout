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

#include <osmscout/AreaAreaIndex.h>

#include <algorithm>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

//#define ANALYZE_CACHE

namespace osmscout {

  const char* AreaAreaIndex::AREA_AREA_IDX="areaarea.idx";

  AreaAreaIndex::AreaAreaIndex(size_t cacheSize)
  : maxLevel(0),
    topLevelOffset(0),
    indexCache(cacheSize)
  {
    // no code
  }

  AreaAreaIndex::~AreaAreaIndex()
  {
    Close();
  }

  void AreaAreaIndex::Close()
  {
    try  {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
    }
  }

  bool AreaAreaIndex::GetIndexCell(uint32_t level,
                                   FileOffset offset,
                                   IndexCell &indexCell,
                                   FileOffset &dataOffset) const
  {
    if (level<maxLevel) {
      std::lock_guard<std::mutex> guard(lookupMutex);
      IndexCache::CacheRef        cacheRef;

#if defined(ANALYZE_CACHE)
      if (indexCache.GetSize()==indexCache.GetMaxSize()) {
        log.Warn() << "areaarea.index cache of " << indexCache.GetSize() << "/" << indexCache.GetMaxSize()<< " is too small";
        indexCache.DumpStatistics("areaarea.idx",IndexCacheValueSizer());
      }
#endif

      if (!indexCache.GetEntry(offset,cacheRef)) {
        IndexCache::CacheEntry cacheEntry(offset);

        cacheRef=indexCache.SetEntry(cacheEntry);

        scanner.SetPos(offset);

        for (size_t c=0; c<4; c++) {
          FileOffset childOffset;

          scanner.ReadNumber(childOffset);

          if (childOffset==0) {
            cacheRef->value.children[c]=0;
          }
          else {
            cacheRef->value.children[c]=offset-childOffset;
          }
        }

        cacheRef->value.data=scanner.GetPos();

        indexCell=cacheRef->value;
      }
      else {
        indexCell=cacheRef->value;
      }
    }
    else {
      indexCell.data=offset;

      for (size_t c=0; c<4; c++) {
        indexCell.children[c]=0;
      }
    }

    dataOffset=indexCell.data;

    return true;
  }

  bool AreaAreaIndex::ReadCellData(const TypeConfig& typeConfig,
                                   const TypeInfoSet& types,
                                   FileOffset dataOffset,
                                   std::vector<DataBlockSpan>& spans) const
  {
    std::lock_guard<std::mutex> guard(lookupMutex);

    scanner.SetPos(dataOffset);

    uint32_t typeCount;

    scanner.ReadNumber(typeCount);

    FileOffset prevDataFileOffset=0;

    for (size_t t=0; t<typeCount; t++) {
      TypeId     typeId;
      uint32_t   dataCount;
      FileOffset dataFileOffset;

      scanner.ReadTypeId(typeId,typeConfig.GetAreaTypeIdBytes());
      scanner.ReadNumber(dataCount);
      scanner.ReadNumber(dataFileOffset);

      dataFileOffset+=prevDataFileOffset;
      prevDataFileOffset=dataFileOffset;

      if (dataFileOffset==0) {
        continue;
      }

      TypeInfoRef type=typeConfig.GetAreaTypeInfo(typeId);

      if (types.IsSet(type)) {
        DataBlockSpan span;

        span.startOffset=dataFileOffset;
        span.count=dataCount;

        spans.push_back(span);
      }
    }

    return true;
  }

  void AreaAreaIndex::PushCellsForNextLevel(double minlon,
                                            double minlat,
                                            double maxlon,
                                            double maxlat,
                                            const IndexCell& cellIndexData,
                                            const CellDimension& cellDimension,
                                            size_t cx,
                                            size_t cy,
                                            std::vector<CellRef>& nextCellRefs) const
  {
    if (cellIndexData.children[0]!=0) {
      // top left
      double x=cx*cellDimension.width;
      double y=(cy+1)*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[0],cx,cy+1));
      }
    }

    if (cellIndexData.children[1]!=0) {
      // top right
      double x=(cx+1)*cellDimension.width;
      double y=(cy+1)*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[1],cx+1,cy+1));
      }
    }

    if (cellIndexData.children[2]!=0) {
      // bottom left
      double x=cx*cellDimension.width;
      double y=cy*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[2],cx,cy));
      }
    }

    if (cellIndexData.children[3]!=0) {
      // bottom right
      double x=(cx+1)*cellDimension.width;
      double y=cy*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[3],cx+1,cy));
      }
    }
  }

  bool AreaAreaIndex::Open(const std::string& path, bool memoryMappedData)
  {
    datafilename=AppendFileToDir(path,AREA_AREA_IDX);

    try {
      scanner.Open(datafilename,FileScanner::FastRandom,memoryMappedData);

      scanner.ReadNumber(maxLevel);
      scanner.ReadFileOffset(topLevelOffset);

      return !scanner.HasError();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  /**
   * Returns references in form of DataBlockSpans to all areas within the
   * given area,
   *
   * @param typeConfig
   *    Type configuration
   * @param maxLevel
   *    The maximum index level to load areas from
   * @param types
   *    Set of types to load data for
   * @param maxCount
   *    Maximum number of elements to return
   * @param spans
   *    List of DataBlockSpans referencing the the found areas
   */
  bool AreaAreaIndex::GetAreasInArea(const TypeConfig& typeConfig,
                                     const GeoBox& boundingBox,
                                     size_t maxLevel,
                                     const TypeInfoSet& types,
                                     std::vector<DataBlockSpan>& spans,
                                     TypeInfoSet& loadedTypes) const
  {
    StopClock            time;
    std::vector<CellRef> cellRefs;     // cells to scan in this level
    std::vector<CellRef> nextCellRefs; // cells to scan for the next level
    double               minlon=boundingBox.GetMinLon()+180.0;
    double               maxlon=boundingBox.GetMaxLon()+180.0;
    double               minlat=boundingBox.GetMinLat()+90.0;
    double               maxlat=boundingBox.GetMaxLat()+90.0;

    // Clear result data structures
    spans.clear();
    loadedTypes.Clear();

    // Make the vector preallocate memory for the expected data size
    // This should void reallocation
    spans.reserve(1000);

    cellRefs.reserve(2000);
    nextCellRefs.reserve(2000);

    cellRefs.push_back(CellRef(topLevelOffset,0,0));

    try {
      // For all levels:
      // * Take the tiles and offsets of the last level
      // * Calculate the new tiles and offsets that still interfere with given area
      // * Add the new offsets to the list of offsets and finish if we have
      //   reached maxLevel or maxAreaCount.
      // * copy no, ntx, nty to ctx, cty, co and go to next iteration
      for (uint32_t level=0;
           level<=this->maxLevel &&
           level<=maxLevel &&
           !cellRefs.empty();
           level++) {
        nextCellRefs.clear();

        for (const auto& cellRef : cellRefs) {
          IndexCell  cellIndexData;
          FileOffset cellDataOffset;

          if (!GetIndexCell(level,
                            cellRef.offset,
                            cellIndexData,
                            cellDataOffset)) {
            log.Error() << "Cannot find offset " << cellRef.offset << " in level " << level << " in file '" << scanner.GetFilename() << "'";
            return false;
          }

          // Now read the area offsets by type in this index entry

          if (!ReadCellData(typeConfig,
                            types,
                            cellDataOffset,
                            spans)) {
            log.Error() << "Cannot read index data for level " << level << " at offset " << cellDataOffset << " in file '" << scanner.GetFilename() << "'";
            return false;
          }

          if (level<this->maxLevel) {
            size_t cx=cellRef.x*2;
            size_t cy=cellRef.y*2;

            PushCellsForNextLevel(minlon,
                                  minlat,
                                  maxlon,
                                  maxlat,
                                  cellIndexData,
                                  cellDimension[level+1],
                                  cx,cy,
                                  nextCellRefs);
          }
        }

        std::swap(cellRefs,nextCellRefs);
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    time.Stop();

    if (time.GetMilliseconds()>100) {
      log.Warn() << "Retrieving " << spans.size() << " spans from area index for " << boundingBox.GetDisplayText() << " took " << time.ResultString();
    }

    loadedTypes=types;

    return true;
  }

  void AreaAreaIndex::DumpStatistics()
  {
    indexCache.DumpStatistics(AREA_AREA_IDX,IndexCacheValueSizer());
  }
}
