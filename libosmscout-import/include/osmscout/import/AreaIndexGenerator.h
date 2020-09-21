#ifndef LIBOSMSCOUT_AREAINDEXGENERATOR_H
#define LIBOSMSCOUT_AREAINDEXGENERATOR_H

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

#include <list>
#include <map>

#include <osmscout/Pixel.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/TileId.h>
#include <osmscout/util/String.h>
#include <osmscout/util/File.h>
#include <osmscout/system/Compiler.h>
#include <osmscout/TypeInfoSet.h>

namespace osmscout {

  /**
   * Generic Area index generator
   */
  template <typename Object>
  class AreaIndexGenerator : public ImportModule
  {
  protected:
    typedef std::map<TileId,size_t>                 CoordCountMap;
    typedef std::map<TileId,std::list<FileOffset> > CoordOffsetsMap;

    struct TypeData
    {
      MagnificationLevel indexLevel;   //! magnification level of index
      size_t             indexCells;   //! Number of filled cells in index
      size_t             indexEntries; //! Number of entries over all cells

      TileIdBox          tileBox;

      FileOffset         indexOffset; //! Position in file where the offset of the bitmap is written to

      TypeData();

      inline bool HasEntries()
      {
        return indexCells>0 &&
               indexEntries>0;
      }
    };

    std::string typeName;
    std::string typeNamePlural;
    std::string dataFile;
    std::string indexFile;

  protected:
    AreaIndexGenerator(const std::string &typeName,
                       const std::string &typeNamePlural,
                       const std::string &dataFile,
                       const std::string &indexFile):
       typeName(typeName),
       typeNamePlural(typeNamePlural),
       dataFile(dataFile),
       indexFile(indexFile)
    {}

    virtual bool FitsIndexCriteria(Progress& progress,
                                   const TypeInfo& typeInfo,
                                   const CoordCountMap& cellFillCount) const;

    /**
     * Calculate internal statistics that are the base for deciding if the given object type is
     * indexed at the given index level.
     *
     * @param level
     * @param typeData
     * @param cellFillCount
     */
    void CalculateStatistics(const MagnificationLevel& level,
                             TypeData& typeData,
                             const CoordCountMap& cellFillCount) const;

    bool CalculateDistribution(const TypeConfig& typeConfig,
                               const ImportParameter& parameter,
                               Progress& progress,
                               const std::vector<TypeInfoRef>& types,
                               std::vector<TypeData>& typeData,
                               const MagnificationLevel& minLevelParam,
                               const MagnificationLevel& maxLevelParam,
                               bool useMmap,
                               MagnificationLevel& maxLevel) const;

    /**
     * For each cell we store a file offset to the bitmap data or 0, if there is no data for the cell. The bitmap entry itself
     * contains the number of offsets followed by the offsets themselves (delta-encoded).
     *
     *
     * @param progress
     * @param writer
     * @param typeInfo
     * @param typeData
     * @param typeCellOffsets
     * @return
     */
    bool WriteBitmap(Progress& progress,
                     FileWriter& writer,
                     const TypeInfo& typeInfo,
                     const TypeData& typeData,
                     const CoordOffsetsMap& typeCellOffsets);

    virtual void WriteTypeId(const TypeConfigRef& typeConfig,
                             const TypeInfoRef &type,
                             FileWriter &writer) const = 0;

    bool MakeAreaIndex(const TypeConfigRef& typeConfig,
                       const ImportParameter& parameter,
                       Progress& progress,
                       const std::vector<TypeInfoRef> &types,
                       const MagnificationLevel &areaIndexMinMag,
                       const MagnificationLevel &areaIndexMaxLevel,
                       bool useMmap);
  };

  template <typename Object>
  AreaIndexGenerator<Object>::TypeData::TypeData()
    : indexLevel(0),
      indexCells(0),
      indexEntries(0),
      tileBox(TileId(0,0),
              TileId(0,0)),
      indexOffset(0)
  {
    // no code
  }

  template <typename Object>
  bool AreaIndexGenerator<Object>::WriteBitmap(Progress& progress,
                                               FileWriter& writer,
                                               const TypeInfo& typeInfo,
                                               const TypeData& typeData,
                                               const CoordOffsetsMap& typeCellOffsets)
  {
    size_t indexEntries=0;
    size_t dataSize=0;
    char   buffer[10];

    //
    // Calculate the number of entries and the overall size of the data in the bitmap entries
    // We need the overall size of the bitmap entry data, because we would store the file offset only with
    // that much bytes we need to address the last data entry.

    for (const auto& cell : typeCellOffsets) {
      indexEntries+=cell.second.size();

      dataSize+=EncodeNumber(cell.second.size(),
                             buffer);

      FileOffset previousOffset=0;

      for (const auto& offset : cell.second) {
        FileOffset data=offset-previousOffset;

        dataSize+=EncodeNumber(data,buffer);

        previousOffset=offset;
      }
    }

    // "+1" because we add +1 to every offset, to generate offset > 0
    uint8_t dataOffsetBytes=BytesNeededToEncodeNumber(dataSize+1);

    progress.Info("Writing map for "+
                  typeInfo.GetName()+" , "+
                  ByteSizeToString(1.0*dataOffsetBytes*typeData.tileBox.GetCount()+dataSize));

    FileOffset bitmapOffset;

    bitmapOffset=writer.GetPos();

    assert(typeData.indexOffset!=0);

    writer.SetPos(typeData.indexOffset);

    writer.WriteFileOffset(bitmapOffset);
    writer.Write(dataOffsetBytes);

    writer.SetPos(bitmapOffset);

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only overwrite cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<typeData.tileBox.GetCount(); i++) {
      writer.WriteFileOffset(0,
                             dataOffsetBytes);
    }

    FileOffset dataStartOffset;

    dataStartOffset=writer.GetPos();

    // Now write the list of offsets of objects for every cell with content
    for (const auto& cell : typeCellOffsets) {
      FileOffset bitmapCellOffset=bitmapOffset+
                                  ((cell.first.GetY()-typeData.tileBox.GetMinY())*typeData.tileBox.GetWidth()+
                                   cell.first.GetX()-typeData.tileBox.GetMinX())*(FileOffset)dataOffsetBytes;
      FileOffset previousOffset=0;
      FileOffset cellOffset;

      assert(bitmapCellOffset>=bitmapOffset);

      cellOffset=writer.GetPos();

      writer.SetPos(bitmapCellOffset);

      assert(cellOffset>bitmapCellOffset);

      // We add +1 to make sure, that we can differentiate between "0" as "no entry" and "0" as first data entry.
      writer.WriteFileOffset(cellOffset-dataStartOffset+1,dataOffsetBytes);

      writer.SetPos(cellOffset);

      writer.WriteNumber((uint32_t)cell.second.size());

      // FileOffsets are already in increasing order, since
      // File is scanned from start to end
      for (const auto& offset : cell.second) {
        assert(offset>previousOffset);

        writer.WriteNumber((FileOffset)(offset-previousOffset));

        previousOffset=offset;
      }
    }

    return true;
  }

  template <typename Object>
  bool AreaIndexGenerator<Object>::FitsIndexCriteria(Progress& progress,
                                                     const TypeInfo& typeInfo,
                                                     const CoordCountMap& cellFillCount) const
  {
    if (cellFillCount.empty()) {
      return true;
    }

    size_t overallCount=0;
    size_t maxCellCount=0;

    for (const auto& cell : cellFillCount) {
      overallCount+=cell.second;
      maxCellCount=std::max(maxCellCount,cell.second);
    }

    // Average number of entries per tile cell
    double average=overallCount*1.0/cellFillCount.size();

    size_t emptyCount=0;
    size_t toLowCount=0;
    size_t toHighCount=0;
    size_t inCount=0;
    size_t allCount=0;

    for (const auto& cell : cellFillCount) {
      if (cell.second==0) {
        emptyCount++;
      }
      else if (cell.second<0.4*average) {
        toLowCount++;
      }
      else if (cell.second>128){
        toHighCount++;
      }
      else {
        inCount++;
      }

      allCount++;
    }

    if (toHighCount*1.0/allCount>=0.05) {
      return false;
    }

    if (toLowCount*1.0/allCount>=0.2) {
      progress.Warning(typeInfo.GetName()+" has more than 20% cells with <40% of average filling ("+std::to_string(toLowCount)+"/"+std::to_string(allCount)+")");
    }

    /*
    // If the fill rate of the index is too low, we use this index level anyway
    if (fillRate<parameter.GetAreaWayIndexMinFillRate()) {
      progress.Warning(typeInfo.GetName()+" is not well distributed");
      return true;
    }

    // If average fill size and max fill size for tile cells
    // is within limits, store it now.
    if (maxCellCount<=parameter.GetAreaWayIndexCellSizeMax() &&
        average<=parameter.GetAreaWayIndexCellSizeAverage()) {
      return true;
    }*/

    return true;
  }

  template <typename Object>
  bool AreaIndexGenerator<Object>::MakeAreaIndex(const TypeConfigRef& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 const std::vector<TypeInfoRef> &types,
                                                 const MagnificationLevel &areaIndexMinMag,
                                                 const MagnificationLevel &areaIndexMaxLevel,
                                                 bool useMmap)
  {
    using namespace std::string_literals;

    FileScanner           scanner;
    FileWriter            writer;
    std::vector<TypeData> typeData;
    MagnificationLevel    maxLevel;

    progress.Info("Minimum magnification: "s + areaIndexMinMag);

    //
    // Scanning distribution
    //

    progress.SetAction("Scanning level distribution of "s + typeName + " types"s);

    if (!CalculateDistribution(*typeConfig,
                               parameter,
                               progress,
                               types,
                               typeData,
                               areaIndexMinMag,
                               areaIndexMaxLevel,
                               useMmap,
                               maxLevel)) {
      return false;
    }

    // Calculate number of types which have data

    auto indexEntries=std::count_if(types.begin(),
                                    types.end(),
                                    [&typeData](const TypeInfoRef& type) {
                                      return typeData[type->GetIndex()].HasEntries();
                                    });

    //
    // Writing index file
    //

    progress.SetAction("Generating '"s + indexFile + "'"s);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),indexFile));

      writer.Write((uint32_t)indexEntries);

      for (const auto &type : types) {
        size_t i=type->GetIndex();

        if (typeData[i].HasEntries()) {
          uint8_t    dataOffsetBytes=0;
          FileOffset bitmapOffset=0;

          WriteTypeId(typeConfig,
                      type,
                      writer);

          typeData[i].indexOffset=writer.GetPos();

          writer.WriteFileOffset(bitmapOffset);
          writer.Write(dataOffsetBytes);
          writer.WriteNumber(typeData[i].indexLevel);
          writer.WriteNumber(typeData[i].tileBox.GetMinX());
          writer.WriteNumber(typeData[i].tileBox.GetMaxX());
          writer.WriteNumber(typeData[i].tileBox.GetMinY());
          writer.WriteNumber(typeData[i].tileBox.GetMaxY());
        }
      }

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   dataFile),
                   FileScanner::Sequential,
                   useMmap);

      for (MagnificationLevel l=areaIndexMinMag; l<=maxLevel; l++) {
        Magnification magnification(l);
        TypeInfoSet   indexTypes(*typeConfig);
        uint32_t      objectCount;

        scanner.GotoBegin();

        for (const auto &type : types) {
          if (typeData[type->GetIndex()].HasEntries() &&
              typeData[type->GetIndex()].indexLevel==l) {
            indexTypes.Set(type);
          }
        }

        if (indexTypes.Empty()) {
          continue;
        }

        progress.Info("Scanning "s + typeNamePlural + " for index level "s + l);

        std::vector<CoordOffsetsMap> typeCellOffsets(typeConfig->GetTypeCount());

        scanner.Read(objectCount);

        Object obj;

        for (uint32_t w=1; w <= objectCount; w++) {
          progress.SetProgress(w, objectCount);

          FileOffset offset;

          offset=scanner.GetPos();

          obj.Read(*typeConfig,
                   scanner);

          if (!indexTypes.IsSet(obj.GetType())) {
            continue;
          }

          TileIdBox box(magnification, obj.GetBoundingBox());

          for (const auto& tileId : box) {
            typeCellOffsets[obj.GetType()->GetIndex()][tileId].push_back(offset);
          }
        }

        for (const auto &type : indexTypes) {
          size_t index=type->GetIndex();

          if (!WriteBitmap(progress,
                           writer,
                           *typeConfig->GetTypeInfo(index),
                           typeData[index],
                           typeCellOffsets[index])) {
            return false;
          }
        }
      }

      scanner.Close();
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  template <typename Object>
  void AreaIndexGenerator<Object>::CalculateStatistics(const MagnificationLevel& level,
                                                  TypeData& typeData,
                                                  const CoordCountMap& cellFillCount) const
  {
    // Initialize/reset data structure
    typeData.indexLevel=level;
    typeData.indexCells=cellFillCount.size();
    typeData.indexEntries=0;

    // If we do not have any entries, we are done ;-)
    if (cellFillCount.empty()) {
      return;
    }

    typeData.tileBox=TileIdBox(cellFillCount.begin()->first,cellFillCount.begin()->first);

    for (const auto& cell : cellFillCount) {
      typeData.indexEntries+=cell.second;

      typeData.tileBox=typeData.tileBox.Include(cell.first);
    }
  }

  template <typename Object>
  bool AreaIndexGenerator<Object>::CalculateDistribution(const TypeConfig& typeConfig,
                                                         const ImportParameter& parameter,
                                                         Progress& progress,
                                                         const std::vector<TypeInfoRef>& types,
                                                         std::vector<TypeData>& typeData,
                                                         const MagnificationLevel& minLevelParam,
                                                         const MagnificationLevel& maxLevelParam,
                                                         bool useMmap,
                                                         MagnificationLevel& maxLevel) const
  {
    FileScanner        scanner;
    TypeInfoSet        remainingObjectTypes;
    MagnificationLevel level=minLevelParam;

    maxLevel=MagnificationLevel(0);
    typeData.resize(typeConfig.GetTypeCount());

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   dataFile),
                   FileScanner::Sequential,
                   useMmap);

      remainingObjectTypes.Set(types);

      while (!remainingObjectTypes.Empty() &&
             level <= maxLevelParam) {
        Magnification              magnification(level);
        uint32_t                   objectCount=0;
        TypeInfoSet                currentObjectTypes(remainingObjectTypes);
        std::vector<CoordCountMap> cellFillCount(typeConfig.GetTypeCount());

        progress.Info("Scanning Level " + level + " (" + std::to_string(remainingObjectTypes.Size()) + " types remaining)");

        scanner.GotoBegin();

        scanner.Read(objectCount);

        Object obj;

        for (uint32_t objI=1; objI <= objectCount; objI++) {
          progress.SetProgress(objI, objectCount);

          obj.Read(typeConfig,
                   scanner);

          // Count number of entries per current type and coordinate
          if (!currentObjectTypes.IsSet(obj.GetType())) {
            continue;
          }

          GeoBox boundingBox=obj.GetBoundingBox();

          TileIdBox box(TileId::GetTile(magnification,boundingBox.GetMinCoord()),
                        TileId::GetTile(magnification,boundingBox.GetMaxCoord()));

          for (const auto& tileId : box) {
            cellFillCount[obj.GetType()->GetIndex()][tileId]++;
          }
        }

        // Check if cell fill for current type is in defined limits
        for (auto &type : currentObjectTypes) {
          size_t typeIndex=type->GetIndex();

          if (!FitsIndexCriteria(progress,
                                 *typeConfig.GetTypeInfo(typeIndex),
                                 cellFillCount[typeIndex])) {
            if (level < maxLevelParam) {
              currentObjectTypes.Remove(type);
            }
            else {
              progress.Warning(typeConfig.GetTypeInfo(typeIndex)->GetName()+" has too many index cells, that area filled over the limit");
            }
          }
        }

        for (const auto &type : currentObjectTypes) {
          size_t typeIndex=type->GetIndex();

          CalculateStatistics(level,
                              typeData[typeIndex],
                              cellFillCount[typeIndex]);

          maxLevel=std::max(maxLevel,level);

          progress.Info("Type " + type->GetName() + ", " +
                        std::to_string(typeData[type->GetIndex()].indexCells) + " cells, " +
                        std::to_string(typeData[type->GetIndex()].indexEntries) + " objects");

          remainingObjectTypes.Remove(type);
        }

        level++;
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

}

#endif //LIBOSMSCOUT_AREAINDEXGENERATOR_H
