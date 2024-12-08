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

#include <osmscoutimport/Import.h>

#include <list>
#include <map>
#include <utility>

#include <osmscout/Pixel.h>

#include <osmscout/TypeInfoSet.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileWriter.h>

#include <osmscout/util/TileId.h>
#include <osmscout/util/String.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Generic Area index generator
   */
  template <typename Object>
  class AreaIndexGenerator : public ImportModule
  {
  protected:
    using CoordCountMap = std::map<TileId, size_t>;
    using CoordOffsetsMap = std::map<TileId, std::list<FileOffset>>;

    struct TypeData
    {
      MagnificationLevel indexLevel{0};  //! magnification level of index
      size_t             indexCells=0;   //! Number of filled cells in index
      size_t             indexEntries=0; //! Number of entries over all cells

      TileIdBox          tileBox{TileId(0,0),TileId(0,0)};

      FileOffset         indexOffset=0; //! Position in file where the offset of the bitmap is written to

      inline bool HasEntries()
      {
        return indexCells>0 &&
               indexEntries>0;
      }
    };

  private:
    std::string typeName;
    std::string typeNamePlural;
    std::string dataFile;
    std::string indexFile;

  protected:
    AreaIndexGenerator(const std::string& typeName,
                       const std::string& typeNamePlural,
                       const std::string& dataFile,
                       const std::string& indexFile):
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
     */
    void WriteBitmap(Progress& progress,
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
                       const MagnificationLevel &areaIndexMaxMag,
                       bool useMmap);
  };

  template <typename Object>
  void AreaIndexGenerator<Object>::WriteBitmap(Progress& progress,
                                               FileWriter& writer,
                                               const TypeInfo& typeInfo,
                                               const TypeData& typeData,
                                               const CoordOffsetsMap& typeCellOffsets)
  {
    size_t              dataSize=0;
    std::array<char,10> buffer;

    //
    // Calculate the number of entries and the overall size of the data in the bitmap entries
    // We need the overall size of the bitmap entry data, because we would store the file offset only with
    // that much bytes we need to address the last data entry.

    for (const auto& cell : typeCellOffsets) {
      dataSize+=EncodeNumber(cell.second.size(),
                             buffer);

      FileOffset previousOffset=0;

      for (const auto& offset : cell.second) {
        FileOffset data=offset-previousOffset;

        dataSize+=EncodeNumber(data,
                               buffer);

        previousOffset=offset;
      }
    }

    // "+1" because we add +1 to every offset, to generate offset > 0
    uint8_t dataOffsetBytes=BytesNeededToEncodeNumber(dataSize+1);


    GeoBox boundingBox=typeData.tileBox.GetCenter().GetBoundingBox(typeData.indexLevel);

    progress.Info("Writing map for "+
                  typeInfo.GetName()+
                  " ("+
                  ByteSizeToString(1.0*dataOffsetBytes*typeData.tileBox.GetCount()+dataSize)+", "+
                  GetEllipsoidalDistance(boundingBox.GetTopLeft(),boundingBox.GetBottomRight()).AsString()+", "+
                  std::to_string(typeData.indexEntries/typeData.indexCells)+"/cell"+
                  ")");

    FileOffset bitmapOffset=writer.GetPos();

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

    FileOffset dataStartOffset=writer.GetPos();

    // Now write the list of offsets of objects for every cell with content
    for (const auto& cell : typeCellOffsets) {
      FileOffset bitmapCellOffset=bitmapOffset+
                                  ((cell.first.GetY()-typeData.tileBox.GetMinY())*typeData.tileBox.GetWidth()+
                                   cell.first.GetX()-typeData.tileBox.GetMinX())*(FileOffset)dataOffsetBytes;
      FileOffset previousOffset=0;

      assert(bitmapCellOffset>=bitmapOffset);

      FileOffset cellOffset=writer.GetPos();

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
    double average=double(overallCount)/double(cellFillCount.size());

    size_t emptyCount=0;
    size_t tooLowCount=0;
    size_t tooHighCount=0;
    size_t muchTooHighCount=0;
    size_t okCount=0;
    size_t allCount=0;

    size_t tooLowValue=4*average/10;
    size_t tooHighValue=64+32;
    size_t muchTooHighValue=128+64;

    for (const auto& cell : cellFillCount) {
      allCount++;

      if (cell.second==0) {
        emptyCount++;
      }
      else if (cell.second<tooLowValue) {
        tooLowCount++;
      }
      else if (cell.second>muchTooHighValue) {
        muchTooHighCount++;
      }
      else if (cell.second>tooHighValue) {
        tooHighCount++;
      }
      else {
        okCount++;
      }
    }

    progress.Info(typeInfo.GetName()+" "+
                  std::to_string(emptyCount)+" | "+
                  std::to_string(tooLowCount)+" < "+
                  std::to_string(okCount)+" < "+
                  std::to_string(tooHighCount)+" *"+
                  std::to_string(muchTooHighCount)+"* - "+
                  std::to_string(allCount));

    if (double(muchTooHighCount) / double(allCount) >= 0.01) {
      progress.Warning(typeInfo.GetName() + " has more than 1% cells with much too high entry count, will use smaller tile size");
      return false;
    }

    if (double(tooHighCount) / double(allCount) >= 0.05) {
      progress.Warning(typeInfo.GetName() + " has more than 5% cells with too high entry count, will use smaller tile size");
      return false;
    }

    if (double(tooLowCount) / double(allCount) >= 0.2) {
      progress.Warning(typeInfo.GetName() + " has more than 20% cells with <40% of average filling");
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
                                                 const MagnificationLevel &areaIndexMaxMag,
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

    progress.SetAction("Scanning level distribution of {} type",typeName);

    if (!CalculateDistribution(*typeConfig,
                               parameter,
                               progress,
                               types,
                               typeData,
                               areaIndexMinMag,
                               areaIndexMaxMag,
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

    progress.SetAction("Generating '{}'",indexFile);

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

      for (MagnificationLevel l=areaIndexMinMag; l <= maxLevel; l++) {
        Magnification magnification(l);
        TypeInfoSet   indexTypes(*typeConfig);

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

        uint32_t objectCount=scanner.ReadUInt32();

        Object obj;

        for (uint32_t w=1; w <= objectCount; w++) {
          progress.SetProgress(w, objectCount);

          FileOffset offset=scanner.GetPos();

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

          WriteBitmap(progress,
                      writer,
                      *typeConfig->GetTypeInfo(index),
                      typeData[index],
                      typeCellOffsets[index]);
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
        TypeInfoSet                currentObjectTypes(remainingObjectTypes);
        std::vector<CoordCountMap> cellFillCount(typeConfig.GetTypeCount());

        progress.Info("Scanning Level " + level + " (" + std::to_string(remainingObjectTypes.Size()) + " types remaining)");

        scanner.GotoBegin();

        uint32_t objectCount=scanner.ReadUInt32();

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
        for (const auto &type : currentObjectTypes) {
          size_t typeIndex=type->GetIndex();

          if (!FitsIndexCriteria(progress,
                                 *typeConfig.GetTypeInfo(typeIndex),
                                 cellFillCount[typeIndex])) {
            if (level < maxLevelParam) {
              currentObjectTypes.Remove(type);
            }
            else {
              progress.Warning(typeConfig.GetTypeInfo(typeIndex)->GetName()+" still does not fit good index criteria");
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
