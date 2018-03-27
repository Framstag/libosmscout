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

#include <osmscout/import/GenCoverageIndex.h>

#include <osmscout/AreaDataFile.h>
#include <osmscout/NodeDataFile.h>
#include <osmscout/WayDataFile.h>

#include <osmscout/CoverageIndex.h>

namespace osmscout {

  static uint32_t cellLevel=14;

  CoverageIndexGenerator::CellSet CoverageIndexGenerator::ScanNodes(const TypeConfigRef& typeConfig,
                                                                    const ImportParameter& parameter,
                                                                    Progress& progress) const
  {
    progress.SetAction("Scanning nodes");

    std::set<Pixel> cells;
    FileScanner     nodeScanner;
    uint32_t        nodeCount;

    nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     NodeDataFile::NODES_DAT),
                     FileScanner::Sequential,
                     true);

    nodeScanner.Read(nodeCount);

    for (uint32_t idx=1; idx<=nodeCount; idx++) {
      progress.SetProgress(idx,
                           nodeCount);

      Node node;

      node.Read(*typeConfig,
                nodeScanner);

      cells.insert(tileCalculator.GetTileId(node.GetCoords()));
    }

    nodeScanner.Close();

    return cells;
  }

  CoverageIndexGenerator::CellSet CoverageIndexGenerator::ScanWays(const TypeConfigRef& typeConfig,
                                                                   const ImportParameter& parameter,
                                                                   Progress& progress) const
  {
    progress.SetAction("Scanning ways");

    std::set<Pixel> cells;
    FileScanner     wayScanner;
    uint32_t        wayCount;

    wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    WayDataFile::WAYS_DAT),
                    FileScanner::Sequential,
                    true);

    wayScanner.Read(wayCount);

    for (uint32_t idx=1; idx<=wayCount; idx++) {
      progress.SetProgress(idx,
                           wayCount);

      Way way;

      way.Read(*typeConfig,
               wayScanner);

      // We currently use the area of the way, since this is simpler than a scan line
      // processing

      GeoBox boundingBox;

      way.GetBoundingBox(boundingBox);

      Pixel bottomLeft=tileCalculator.GetTileId(boundingBox.GetBottomLeft());
      Pixel topRight=tileCalculator.GetTileId(boundingBox.GetTopRight());

      for (uint32_t y=bottomLeft.y; y<=topRight.y; y++) {
        for (uint32_t x=bottomLeft.x; x<=topRight.x; x++) {
          cells.insert(Pixel(x,y));
        }
      }
    }

    wayScanner.Close();

    return cells;
  }

  CoverageIndexGenerator::CellSet CoverageIndexGenerator::ScanAreas(const TypeConfigRef& typeConfig,
                                                                    const ImportParameter& parameter,
                                                                    Progress& progress) const
  {
    progress.SetAction("Scanning areas");

    std::set<Pixel> cells;
    FileScanner     areaScanner;
    uint32_t        areaCount;

    areaScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     AreaDataFile::AREAS_DAT),
                     FileScanner::Sequential,
                     true);

    areaScanner.Read(areaCount);

    for (uint32_t idx=1; idx<=areaCount; idx++) {
      progress.SetProgress(idx,
                           areaCount);

      Area area;

      area.Read(*typeConfig,
                areaScanner);

      GeoBox boundingBox;

      area.GetBoundingBox(boundingBox);

      Pixel bottomLeft=tileCalculator.GetTileId(boundingBox.GetBottomLeft());
      Pixel topRight=tileCalculator.GetTileId(boundingBox.GetTopRight());

      for (uint32_t y=bottomLeft.y; y<=topRight.y; y++) {
        for (uint32_t x=bottomLeft.x; x<=topRight.x; x++) {
          cells.insert(Pixel(x,y));
        }
      }
    }

    areaScanner.Close();

    return cells;
  }

  void CoverageIndexGenerator::WriteIndex(const ImportParameter& parameter,
                                          Progress& progress,
                                          const CoverageIndexGenerator::CellSet& cells)
  {
    FileWriter writer;

    progress.SetAction("Generating 'coverage.idx'");

    progress.Info(std::to_string(cells.size())+" bitmap entries");

    Pixel minCell(std::numeric_limits<uint32_t>::max(),
                  std::numeric_limits<uint32_t>::max());
    Pixel maxCell(std::numeric_limits<uint32_t>::min(),
                  std::numeric_limits<uint32_t>::min());

    for (const auto& cell : cells) {
      minCell.x=std::min(minCell.x,cell.x);
      minCell.y=std::min(minCell.y,cell.y);

      maxCell.x=std::max(maxCell.x,cell.x);
      maxCell.y=std::max(maxCell.y,cell.y);
    }

    uint32_t width=maxCell.x-minCell.x+1;
    uint32_t height=maxCell.y-minCell.y+1;

    progress.Info("Original bitmap boundary: "+
                  std::to_string(minCell.x)+","+std::to_string(minCell.y)+" - "+
                  std::to_string(maxCell.x)+","+std::to_string(maxCell.y)+" "+
                  std::to_string(width)+"x"+std::to_string(height));


    if (width/8!=0) {
      width=(width/8+1)*8;
      maxCell.x=minCell.x+width-1;
    }

    if (height/8!=0) {
      height=(height/8+1)*8;
      maxCell.y=minCell.y+height-1;
    }

    progress.Info("Aligned bitmap boundary: "+
                  std::to_string(minCell.x)+","+std::to_string(minCell.y)+" - "+
                  std::to_string(maxCell.x)+","+std::to_string(maxCell.y)+" "+
                  std::to_string(maxCell.x-minCell.x+1)+"x"+std::to_string(maxCell.y-minCell.y+1));

    std::vector<uint8_t> bitmap((width*height)/8,0);

    for (const auto& cell : cells) {
      size_t bitInMap=((cell.y-minCell.y)*width+(cell.x-minCell.x));
      size_t byteInMap=bitInMap/8;
      size_t bitInByte=bitInMap%8;

      bitmap[byteInMap]=bitmap[byteInMap] | ((uint8_t)1 << bitInByte);
    }

    writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                CoverageIndex::COVERAGE_IDX));

    writer.Write(cellLevel);
    writer.Write(minCell.x);
    writer.Write(minCell.y);
    writer.Write(maxCell.x);
    writer.Write(maxCell.y);

    for (auto b : bitmap) {
      writer.Write(b);
    }

    writer.Close();
  }

  CoverageIndexGenerator::CoverageIndexGenerator()
  : tileCalculator(std::pow(2,cellLevel))
  {}

  void CoverageIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                              ImportModuleDescription& description) const
  {
    description.SetName("CoverageIndexGenerator");
    description.SetDescription("Index of import area coverage");

    description.AddRequiredFile(NodeDataFile::NODES_DAT);
    description.AddRequiredFile(WayDataFile::WAYS_DAT);
    description.AddRequiredFile(AreaDataFile::AREAS_DAT);

    description.AddProvidedFile(CoverageIndex::COVERAGE_IDX);
  }

  bool CoverageIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    progress.Info("Generating bitmap for cell level "+std::to_string((size_t)cellLevel));

    try {
      CellSet nodeCells;
      CellSet wayCells;
      CellSet areaCells;

      nodeCells=ScanNodes(typeConfig,
                          parameter,
                          progress);

      // Ways

      wayCells=ScanWays(typeConfig,
                        parameter,
                        progress);

      // Areas

      areaCells=ScanAreas(typeConfig,
                          parameter,
                          progress);

      // Dump result

      progress.Info(std::to_string(nodeCells.size())+" node bitmap entries");
      progress.Info(std::to_string(wayCells.size())+" way bitmap entries");
      progress.Info(std::to_string(areaCells.size())+" area bitmap entries");

      // Merge the result

      progress.SetAction("Calculating bitmap");

      CellSet cells;

      cells=nodeCells;
      cells.insert(wayCells.begin(),wayCells.end());
      cells.insert(areaCells.begin(),areaCells.end());

      WriteIndex(parameter,
                 progress,
                 cells);
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      return false;
    }

    return true;
  }
}
