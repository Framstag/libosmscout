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

#include <osmscout/import/GenWaterIndex.h>

#include <vector>
#include <iostream>
#include <iomanip>

#include <osmscout/Way.h>

#include <osmscout/DataFile.h>
#include <osmscout/BoundingBoxDataFile.h>
#include <osmscout/CoordDataFile.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/WaterIndex.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/import/Preprocess.h>
#include <osmscout/import/RawCoastline.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/WayDataFile.h>

#if !defined(DEBUG_COASTLINE)
#define DEBUG_COASTLINE
#endif

#if !defined(DEBUG_TILING)
#define DEBUG_TILING
#endif

namespace osmscout {

  std::string WaterIndexGenerator::StateToString(State state) const
  {
    switch (state) {
    case unknown:
      return "unknown";
    case land:
      return "land";
    case water:
      return "water";
    case coast:
      return "coast";
    default:
      return "???";
    }
  }

  std::string WaterIndexGenerator::TypeToString(GroundTile::Type type) const
  {
    switch (type) {
    case unknown:
      return "unknown";
    case land:
      return "land";
    case water:
      return "water";
    case coast:
      return "coast";
    default:
      return "???";
    }
  }

  GroundTile::Coord WaterIndexGenerator::Transform(const GeoCoord& point,
                                                   const Level& level,
                                                   double cellMinLat,
                                                   double cellMinLon,
                                                   bool coast)
  {
    //std::cout << "       " << (coast?"*":"+") << " " << point.GetDisplayText() << std::endl;
    GroundTile::Coord coord(floor((point.GetLon()-cellMinLon)/level.cellWidth*GroundTile::Coord::CELL_MAX+0.5),
                            floor((point.GetLat()-cellMinLat)/level.cellHeight*GroundTile::Coord::CELL_MAX+0.5),
                            coast);

    return coord;
  }

  /**
   * Sets the size of the bitmap and initializes state of all tiles to "unknown"
   */
  void WaterIndexGenerator::Level::SetBox(const GeoCoord& minCoord,
                                          const GeoCoord& maxCoord,
                                          double cellWidth, double cellHeight)
  {
    indexEntryOffset=0;

    this->cellWidth=cellWidth;
    this->cellHeight=cellHeight;

    hasCellData=false;
    defaultCellData=State::unknown;

    indexDataOffset=0;

    cellXStart=(uint32_t)floor((minCoord.GetLon()+180.0)/cellWidth);
    cellXEnd=(uint32_t)floor((maxCoord.GetLon()+180.0)/cellWidth);
    cellYStart=(uint32_t)floor((minCoord.GetLat()+90.0)/cellHeight);
    cellYEnd=(uint32_t)floor((maxCoord.GetLat()+90.0)/cellHeight);

    cellXCount=cellXEnd-cellXStart+1;
    cellYCount=cellYEnd-cellYStart+1;

    uint32_t size=(cellXCount*cellYCount)/4;

    if ((cellXCount*cellYCount)%4>0) {
      size++;
    }

    area.resize(size,0x00);
  }

  bool WaterIndexGenerator::Level::IsInAbsolute(uint32_t x, uint32_t y) const
  {
    return x>=cellXStart &&
           x<=cellXEnd &&
           y>=cellYStart &&
           y<=cellYEnd;
  }

  WaterIndexGenerator::State WaterIndexGenerator::Level::GetState(uint32_t x, uint32_t y) const
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    //assert(index<area.size());

    return (State)((area[index] >> offset) & 3);
  }

  void WaterIndexGenerator::Level::SetState(uint32_t x, uint32_t y, State state)
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    //assert(index<area.size());

    area[index]=(area[index] & ~(3 << offset));
    area[index]=(area[index] | (state << offset));
  }

  void WaterIndexGenerator::Level::SetStateAbsolute(uint32_t x, uint32_t y, State state)
  {
    SetState(x-cellXStart,y-cellYStart,state);
  }

  bool WaterIndexGenerator::LoadCoastlines(const ImportParameter& parameter,
                                           Progress& progress,
                                           std::list<CoastRef>& coastlines)
  {
    progress.SetAction("Scanning for coastlines");
    return LoadRawBoundaries(parameter,
                             progress,
                             coastlines,
                             Preprocess::RAWCOASTLINE_DAT,
                             CoastState::land,
                             CoastState::water);
  }

  bool WaterIndexGenerator::LoadDataPolygon(const ImportParameter& parameter,
                       Progress& progress,
                       std::list<CoastRef>& coastlines)
  {
    progress.SetAction("Scanning data polygon");
    return LoadRawBoundaries(parameter,
                             progress,
                             coastlines,
                             Preprocess::RAWDATAPOLYGON_DAT,
                             CoastState::undefined,
                             CoastState::unknown);
  }

  bool WaterIndexGenerator::LoadRawBoundaries(const ImportParameter& parameter,
                                              Progress& progress,
                                              std::list<CoastRef>& coastlines,
                                              const char* rawFile,
                                              CoastState leftState,
                                              CoastState rightState)
  {
    // We must have coastline type defined
    FileScanner                scanner;
    std::list<RawCoastlineRef> rawCoastlines;

    coastlines.clear();

    try {
      uint32_t coastlineCount=0;
      size_t   wayCoastCount=0;
      size_t   areaCoastCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   rawFile),
                   FileScanner::Sequential,
                   true);

      scanner.Read(coastlineCount);

      progress.Info(NumberToString(coastlineCount)+" coastlines");

      for (uint32_t c=1; c<=coastlineCount; c++) {
        progress.SetProgress(c,coastlineCount);

        RawCoastlineRef coastline=std::make_shared<RawCoastline>();

        coastline->Read(scanner);

        rawCoastlines.push_back(coastline);
      }

      progress.SetAction("Resolving nodes of coastline");

      CoordDataFile coordDataFile;

      if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                              parameter.GetCoordDataMemoryMaped())) {
        progress.Error("Cannot open coord file!");
        return false;
      }

      std::set<OSMId> nodeIds;

      for (const auto& coastline : rawCoastlines) {
        for (size_t n=0; n<coastline->GetNodeCount(); n++) {
          nodeIds.insert(coastline->GetNodeId(n));
        }
      }

      CoordDataFile::ResultMap coordsMap;

      if (!coordDataFile.Get(nodeIds,
                             coordsMap)) {
        progress.Error("Cannot read nodes!");
        return false;
      }

      nodeIds.clear();

      progress.SetAction("Enriching coastline with node data");

      while (!rawCoastlines.empty()) {
        RawCoastlineRef coastline=rawCoastlines.front();
        bool            processingError=false;

        rawCoastlines.pop_front();

        CoastRef coast=std::make_shared<Coast>();

        coast->id=coastline->GetId();
        coast->isArea=coastline->IsArea();

        coast->coast.resize(coastline->GetNodeCount());
        coast->left=leftState;
        coast->right=rightState;

        for (size_t n=0; n<coastline->GetNodeCount(); n++) {
          CoordDataFile::ResultMap::const_iterator coord=coordsMap.find(coastline->GetNodeId(n));

          if (coord==coordsMap.end()) {
            processingError=true;

            progress.Error("Cannot resolve node with id "+
                           NumberToString(coastline->GetNodeId(n))+
                           " for coastline "+
                           NumberToString(coastline->GetId()));

            break;
          }

          if (n==0) {
            coast->frontNodeId=coord->second.GetOSMScoutId();
          }

          if (n==coastline->GetNodeCount()-1) {
            coast->backNodeId=coord->second.GetOSMScoutId();
          }

          coast->coast[n].Set(coord->second.GetSerial(),
                              coord->second.GetCoord());
        }

        if (!processingError) {
          if (coast->isArea) {
            areaCoastCount++;
          }
          else {
            wayCoastCount++;
          }

          coastlines.push_back(coast);
        }
      }

      progress.Info(NumberToString(wayCoastCount)+" way coastline(s), "+NumberToString(areaCoastCount)+" area coastline(s)");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void GetSegmentBoundingBox(const std::vector<Point>& nodes,
                             size_t from, size_t to,
                             GeoBox& boundingBox)
  {
    if (nodes.empty() || from>=to){
      boundingBox.Invalidate();
    }

    double minLon=nodes[from%nodes.size()].GetLon();
    double maxLon=nodes[from%nodes.size()].GetLon();
    double minLat=nodes[from%nodes.size()].GetLat();
    double maxLat=nodes[from%nodes.size()].GetLat();

    for (size_t i=from; i<to; i++) {
      minLon=std::min(minLon,nodes[i%nodes.size()].GetLon());
      maxLon=std::max(maxLon,nodes[i%nodes.size()].GetLon());
      minLat=std::min(minLat,nodes[i%nodes.size()].GetLat());
      maxLat=std::max(maxLat,nodes[i%nodes.size()].GetLat());
    }

    boundingBox.Set(GeoCoord(minLat,minLon),
                    GeoCoord(maxLat,maxLon));
  }

  void computeSegmentBoxes(const std::vector<Point>& path,
                           std::vector<GeoBox> &segmentBoxes,
                           size_t bound)
  {
    for (size_t i=0;i<bound;i+=1000){
      GeoBox box;
      GetSegmentBoundingBox(path,i,std::min(i+1000,bound), box);
      segmentBoxes.push_back(box);
    }
  }

  bool WaterIndexGenerator::FirstPathIntersection(const std::vector<Point> &aPath,
                                                  const std::vector<Point> &bPath,
                                                  bool aClosed,
                                                  bool bClosed,
                                                  size_t &aIndex,
                                                  size_t &bIndex,
                                                  GeoCoord &intersection,
                                                  double &orientation
                                                  )
  {
    size_t aBound=aClosed?aIndex+aPath.size():aPath.size()-1;
    size_t bBound=bClosed?bIndex+bPath.size():bPath.size()-1;
    size_t bStart=bIndex;

    if (bStart>=bBound || aIndex>=aBound){
      return false;
    }

    GeoBox aBox;
    GeoBox bBox;
    GetSegmentBoundingBox(aPath,aIndex,aBound,aBox);
    GetSegmentBoundingBox(bPath,bIndex,bBound,bBox);
    GeoBox aLineBox;

    // compute b-boxes for B path, each 1000 point-long segment
    std::vector<GeoBox> bSegmentBoxes;
    computeSegmentBoxes(bPath,bSegmentBoxes,bBound);

    for (;aIndex<aBound;aIndex++){
      Point a1=aPath[aIndex%aPath.size()];
      Point a2=aPath[(aIndex+1)%aPath.size()];
      aLineBox.Set(a1.GetCoord(),a2.GetCoord());
      if (!bBox.Intersects(aLineBox,/*openInterval*/false)){
        continue;
      }
      for (bIndex=bStart;bIndex<bBound;bIndex++){
        Point b1=bPath[bIndex%bPath.size()];
        Point b2=bPath[(bIndex+1)%bPath.size()];
        
        if ((!bSegmentBoxes[bIndex/1000].Intersects(aLineBox)) &&
             !aLineBox.Intersects(GeoBox(b1.GetCoord(),b2.GetCoord()),/*openInterval*/false)){
          // round up
          bIndex+=(1000-(bIndex%1000));
          continue;
        }
        if (!aLineBox.Intersects(GeoBox(b1.GetCoord(),b2.GetCoord()),/*openInterval*/false)){
          continue;
        }
        if (GetLineIntersection(a1.GetCoord(),a2.GetCoord(),
                                b1.GetCoord(),b2.GetCoord(),
                                intersection)){

          orientation=(intersection.GetLon()-a1.GetLon())*
                      (b2.GetLat()-intersection.GetLat())-
                      (intersection.GetLat()-a1.GetLat())*
                      (b2.GetLon()-intersection.GetLon());
#if defined(DEBUG_TILING)
          std::cout << "    Found intersection " << intersection.GetLat() << " " << intersection.GetLon()
            << " direction " << orientation << std::endl;
#endif
          return true;
        }
      }
    }
    return false;
  }

  void cutPath(std::vector<Point> &dst, const std::vector<Point> &src, size_t start, size_t end)
  {
    start=start%src.size();
    end=end%src.size();
    if (start>end){
      dst.insert(dst.end(),
                 src.begin()+start,
                 src.end());
      dst.insert(dst.end(),
                 src.begin(),
                 src.begin()+end);
    }else{
      dst.insert(dst.end(),
                 src.begin()+start,
                 src.begin()+end);
    }
  }

  void WaterIndexGenerator::SynthetizeCoastlinesSegments(Progress& progress,
                                                         const std::list<CoastRef>& dataPolygons,
                                                         const std::list<CoastRef>& coastlineWays,
                                                         std::list<CoastRef> &coastlines)
  {
    std::list<CoastRef> candidates;

    for (const auto &polygon:dataPolygons){
      CoastRef candidate=std::make_shared<Coast>();
      candidate->isArea=true;
      candidate->coast=polygon->coast;
      candidate->left=polygon->left;
      candidate->right=polygon->right;
      candidates.push_back(candidate);
    }

    while (!candidates.empty()){
      CoastRef c = candidates.front();
      candidates.pop_front();

      for (const auto &w:coastlineWays){

        // try to find intersection between this candidate and way
        size_t ci=0;
        size_t wi=0;
        double orientation;
        GeoCoord intersection1;
        while (FirstPathIntersection(c->coast,w->coast,
                                     c->isArea,false,
                                     ci,wi,
                                     intersection1,
                                     orientation)){
          if (orientation>0.0){ // left agle: candidate -> intersection -> way
            size_t wi2=wi+1;
            size_t ci2=ci+1;
            GeoCoord intersection2;
            if (FirstPathIntersection(w->coast,c->coast,
                                      false,c->isArea,
                                      wi2,ci2,
                                      intersection2,
                                      orientation)){
              if (orientation>0.0){ // left angle: way -> intersection -> candidate

                // cut candidate from intersection2 to intersection1,
                // create left/right boundary of land/unknown
                CoastRef landBoundary=std::make_shared<Coast>();
                landBoundary->coast.push_back(Point(0,intersection2));
                cutPath(landBoundary->coast, c->coast, 
                        (ci2+1), ci);
                landBoundary->coast.push_back(Point(0,intersection1));
                landBoundary->left=w->left;
                landBoundary->right=c->right;
                assert(landBoundary->right==CoastState::unknown);
                assert(landBoundary->left==CoastState::land);
                landBoundary->isArea=false;
                //coastlines.push_back(landBoundary);
                candidates.push_back(landBoundary);
                
                // cut coastline way from intersection 1 to intersection2
                CoastRef shortedWay=std::make_shared<Coast>();
                shortedWay->id=w->id;
                shortedWay->sortCriteria=w->sortCriteria;
                shortedWay->left=w->left;
                shortedWay->right=w->right;
                shortedWay->isArea=false;
                shortedWay->coast.push_back(Point(0,intersection1));
                cutPath(shortedWay->coast, w->coast,
                        wi+1, wi2);
                shortedWay->coast.push_back(Point(0,intersection2));
                coastlines.push_back(shortedWay);

                // finally, cut candidate itself from intersecion1 to intersection2
                std::vector<Point> cutted;
                cutted.push_back(Point(0,intersection1));
                cutPath(cutted, c->coast,
                        ci+1, ci2);
                cutted.push_back(Point(0,intersection2));
                c->coast=cutted;
                c->left=CoastState::water;
                c->isArea=false;
              }
            }else{
              // right angle between coastline and data area should not happen now
              progress.Warning("Right angle between coastline and data polygon "+intersection2.GetDisplayText());
              break;
            }
            wi=wi2+1;
            ci=0;
          }else{
            wi=0;
          }
          ci++;
        }
      }
      
      coastlines.push_back(c);
    }
  }


  void WaterIndexGenerator::MergeCoastlines(Progress& progress,
                                            std::list<CoastRef>& coastlines)
  {
    progress.SetAction("Merging coastlines");

    std::map<Id,CoastRef> coastStartMap;
    std::list<CoastRef>   mergedCoastlines;
    std::set<Id>          blacklist;
    size_t                wayCoastCount=0;
    size_t                areaCoastCount=0;

    std::list<CoastRef>::iterator c=coastlines.begin();
    while( c!=coastlines.end()) {
      CoastRef coast=*c;

      if (coast->isArea) {
        areaCoastCount++;
        mergedCoastlines.push_back(coast);

        c=coastlines.erase(c);
      }
      else {
        coastStartMap.insert(std::make_pair(coast->frontNodeId,coast));

        c++;
      }
    }

    bool merged=true;

    while (merged) {
      merged=false;

      for (const auto& coast : coastlines) {
        if (blacklist.find(coast->id)!=blacklist.end()) {
          continue;
        }

        std::map<Id,CoastRef>::iterator other=coastStartMap.find(coast->backNodeId);

        if (other!=coastStartMap.end() &&
            blacklist.find(other->second->id)==blacklist.end() &&
            coast->id!=other->second->id) {
          for (size_t i=1; i<other->second->coast.size(); i++) {
            coast->coast.push_back(other->second->coast[i]);
          }

          coast->backNodeId=coast->coast.back().GetId();

          // Immediately reduce memory
          other->second->coast.clear();

          blacklist.insert(other->second->id);
          coastStartMap.erase(other);

          merged=true;
        }
      }
    }

    // Gather merged coastlines
    for (const auto& coastline : coastlines) {
      if (blacklist.find(coastline->id)!=blacklist.end()) {
        continue;
      }

      if (coastline->frontNodeId==coastline->backNodeId) {
        coastline->isArea=true;
        coastline->coast.pop_back();

        areaCoastCount++;
      }
      else {
        wayCoastCount++;
      }

      if (coastline->coast.size()<=2) {
        progress.Warning("Dropping to short coastline with id "+NumberToString(coastline->id));
        continue;
      }

      mergedCoastlines.push_back(coastline);
    }

    progress.Info(NumberToString(wayCoastCount)+" way coastline(s), "+NumberToString(areaCoastCount)+" area coastline(s)");

    coastlines=mergedCoastlines;
  }

  /**
   * try to synthetize coastline segments from all way coastlines
   * that intersects with data polygon
   *
   * @param progress
   * @param coastlines
   * @param dataPolygon
   */
  void WaterIndexGenerator::SynthetizeCoastlines(Progress& progress,
                                                 std::list<CoastRef>& coastlines,
                                                 std::list<CoastRef>& dataPolygon)
  {
    if (dataPolygon.empty()){
      return;
    }

    progress.SetAction("Synthetize coastlines");

    std::list<CoastRef> clippedCoastlineSegments;
    std::list<CoastRef> wayCoastlines;
    for (const auto& coastline : coastlines) {
      if (coastline->isArea){
        clippedCoastlineSegments.push_back(coastline);
      }else{
        wayCoastlines.push_back(coastline);
      }
    }

    osmscout::StopClock clock;
    size_t areasBefore=clippedCoastlineSegments.size();
    SynthetizeCoastlinesSegments(progress,
                                 dataPolygon,
                                 wayCoastlines,
                                 clippedCoastlineSegments);

    // define coastline states if there are still some undefined
    for (auto const &coastline:clippedCoastlineSegments){
      if (coastline->right==CoastState::undefined){
        coastline->right=CoastState::unknown;
      }
      if (coastline->left==CoastState::undefined && coastline->isArea){
        for (auto const &testCoast:clippedCoastlineSegments){
          if (testCoast->right==CoastState::water &&
              IsAreaAtLeastPartlyInArea(testCoast->coast,coastline->coast)){
            coastline->left=CoastState::water;
          }
        }
      }
      if (coastline->left==CoastState::undefined){
        // still undefined, it is land probably
        coastline->left=CoastState::land;
      }
    }

    clock.Stop();
    progress.Info(NumberToString(dataPolygon.size())+" data polygon(s), and "+
                  NumberToString(wayCoastlines.size())+" way coastline(s) synthetized into "+
                  NumberToString(clippedCoastlineSegments.size()-areasBefore)+" new area coastlines(s), tooks "+
                  clock.ResultString() +" s"
    );

    coastlines=clippedCoastlineSegments;
  }

  /**
   * Markes a cell as "coast", if one of the coastlines intersects with it.
   *
   */
  void WaterIndexGenerator::MarkCoastlineCells(Progress& progress,
                                               Level& level,
                                               const Data& data)
  {
    progress.Info("Marking cells containing coastlines");

    for (const auto& coastline : data.coastlines) {
      // Marks cells on the path as coast
      std::set<Pixel> coords;

      GetCells(level,
               coastline->points,
               coords);

      for (const auto& coord : coords) {
        if (level.IsInAbsolute(coord.x,coord.y)) {
          if (level.GetState(coord.x-level.cellXStart,coord.y-level.cellYStart)==unknown) {
#if defined(DEBUG_TILING)
            std::cout << "Coastline: " << coord.x-level.cellXStart << "," << coord.y-level.cellYStart << " " << coastline->id << std::endl;
#endif
            level.SetStateAbsolute(coord.x,coord.y,coast);
          }
        }
      }

    }
  }

  /**
   * Calculate the cell type for cells directly around coast cells
   * @param progress
   * @param level
   * @param cellGroundTileMap
   */
  void WaterIndexGenerator::CalculateCoastEnvironment(Progress& progress,
                                                      Level& level,
                                                      const std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap)
  {
    progress.Info("Calculate coast cell environment");

    for (const auto& tileEntry : cellGroundTileMap) {
      Pixel coord=tileEntry.first;
      State  state[4];      // type of the neighbouring cells: top, right, bottom, left
      size_t coordCount[4]; // number of coords on the given line (top, right, bottom, left)

      state[0]=unknown;
      state[1]=unknown;
      state[2]=unknown;
      state[3]=unknown;

      coordCount[0]=0;
      coordCount[1]=0;
      coordCount[2]=0;
      coordCount[3]=0;

      // Preset top
      if (coord.y<level.cellYCount-1) {
        state[0]=level.GetState(coord.x,coord.y+1);
      }

      // Preset right
      if (coord.x<level.cellXCount-1) {
        state[1]=level.GetState(coord.x+1,coord.y);
      }

      // Preset bottom
      if (coord.y>0) {
        state[2]=level.GetState(coord.x,coord.y-1);
      }

      // Preset left
      if (coord.x>0) {
        state[3]=level.GetState(coord.x-1,coord.y);
      }

      // Identify 'land' cells in relation to 'coast' cells
      for (const auto& tile : tileEntry.second) {
        State tileState=State::unknown;
        switch(tile.type){
          case unknown:
            tileState=State::unknown;
            break;
          case land:
            tileState=State::land;
            break;
          case water:
            tileState=State::water;
            break;
          case coast:
            tileState=State::unknown;
            break;
        }
        for (size_t c=0; c<tile.coords.size()-1;c++) {

          //
          // Count number of coords *on* the border
          //

          // top
          if (tile.coords[c].y==GroundTile::Coord::CELL_MAX) {
            coordCount[0]++;
          }
          // right
          else if (tile.coords[c].x==GroundTile::Coord::CELL_MAX) {
            coordCount[1]++;
          }
          // bottom
          else if (tile.coords[c].y==0) {
            coordCount[2]++;
          }
          // left
          else if (tile.coords[c].x==0) {
            coordCount[3]++;
          }

          //
          // Detect fills over a complete border
          //

          // line at the top from left to right => land is above current cell
          if (tile.coords[c].x==0 &&
              tile.coords[c].y==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[0]==unknown) {
              state[0]=tileState;
            }
          }

          // Line from right top to bottom => land is right of current cell
          if (tile.coords[c].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c].y==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c+1].y==0) {
            if (state[1]==unknown) {
              state[1]=tileState;
            }
          }

          // Line a the bottom from right to left => land is below current cell
          if (tile.coords[c].x==GroundTile::Coord::CELL_MAX &&
              tile.coords[c].y==0 &&
              tile.coords[c+1].x==0 &&
              tile.coords[c+1].y==0) {
            if (state[2]==unknown) {
              state[2]=tileState;
            }
          }

          // Line left from bottom to top => land is left of current cell
          if (tile.coords[c].x==0 &&
              tile.coords[c].y==0 &&
              tile.coords[c+1].x==0 &&
              tile.coords[c+1].y==GroundTile::Coord::CELL_MAX) {
            if (state[3]==unknown) {
              state[3]=tileState;
            }
          }
        }
      }

      // top
      if (coord.y<level.cellYCount-1 &&
        level.GetState(coord.x,coord.y+1)==unknown) {
        if (state[0]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[0]) << " above coast: " << coord.x << "," << coord.y+1 << std::endl;
#endif
          level.SetState(coord.x,coord.y+1,state[0]);
        }
      }

      if (coord.x<level.cellXCount-1 &&
        level.GetState(coord.x+1,coord.y)==unknown) {
        if (state[1]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[1]) << " right of coast: " << coord.x+1 << "," << coord.y << std::endl;
#endif
          level.SetState(coord.x+1,coord.y,state[1]);
        }
      }

      if (coord.y>0 &&
        level.GetState(coord.x,coord.y-1)==unknown) {
        if (state[2]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[2]) << " below coast: " << coord.x << "," << coord.y-1 << std::endl;
#endif
          level.SetState(coord.x,coord.y-1,state[2]);
        }
      }

      if (coord.x>0 &&
        level.GetState(coord.x-1,coord.y)==unknown) {
        if (state[3]!=unknown) {
#if defined(DEBUG_TILING)
          std::cout << "Assume " << StateToString(state[3]) << " left of coast: " << coord.x-1 << "," << coord.y << std::endl;
#endif
          level.SetState(coord.x-1,coord.y,state[3]);
        }
      }
    }
  }

  /**
   * Assume cell type 'land' for cells that intersect with 'land' object types
   *
   * Every cell that is unknown but contains a way (that is marked
   * as "to be ignored"), must be land.
   */
  bool WaterIndexGenerator::AssumeLand(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       Level& level)
  {
    progress.Info("Assume land");

    BridgeFeatureReader bridgeFeatureRader(typeConfig);
    TunnelFeatureReader tunnelFeatureRader(typeConfig);
    EmbankmentFeatureReader embankmentFeatureRader(typeConfig);
    FileScanner         scanner;
    uint32_t            wayCount=0;

    // We do not yet know if we handle borders as ways or areas

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      scanner.Read(wayCount);

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        Way way;

        way.Read(typeConfig,
                 scanner);

        if (way.GetType()!=typeConfig.typeInfoIgnore &&
            !way.GetType()->GetIgnoreSeaLand() &&
            !tunnelFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            !bridgeFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            !embankmentFeatureRader.IsSet(way.GetFeatureValueBuffer()) &&
            way.nodes.size()>=2) {
          std::set<Pixel> coords;

          GetCells(level,way.nodes,coords);

          for (const auto& coord : coords) {
            if (level.IsInAbsolute(coord.x,coord.y)) {
              if (level.GetState(coord.x-level.cellXStart,coord.y-level.cellYStart)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Assume land: " << coord.x-level.cellXStart << "," << coord.y-level.cellYStart << " Way " << way.GetFileOffset() << " " << way.GetType()->GetName() << " is defining area as land" << std::endl;
#endif
                level.SetStateAbsolute(coord.x,coord.y,land);
              }
            }
          }
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  /**
   * Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
   *
   * Converts all cells of state "unknown" that touch a tile with state
   * "water" to state "water", too.
   */
  void WaterIndexGenerator::FillWater(Progress& progress,
                                      Level& level,
                                      size_t tileCount,
                                      const std::list<CoastRef>& dataPolygon)
  {
    progress.Info("Filling water");

    for (size_t i=1; i<=tileCount; i++) {
      Level newLevel(level);

      for (uint32_t y=0; y<level.cellYCount; y++) {
        for (uint32_t x=0; x<level.cellXCount; x++) {
          if (level.GetState(x,y)==water) {

            // avoid filling of water outside data polygon
            if (!dataPolygon.empty()){
              CellBoundaries cellBoundary(level,Pixel(x,y));
              std::vector<GeoCoord> cellCoords;
              cellCoords.assign(cellBoundary.borderPoints, cellBoundary.borderPoints + 4);
              bool included=false;
              for (const auto &poly:dataPolygon){
                if (IsAreaAtLeastPartlyInArea(cellCoords,poly->coast)){
                  included=true;
                  break;
                }
              }
              if (!included){
                continue;
              }
            }

            if (y>0) {
              if (level.GetState(x,y-1)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water below water: " << x << "," << y-1 << std::endl;
#endif
                newLevel.SetState(x,y-1,water);
              }
            }

            if (y<level.cellYCount-1) {
              if (level.GetState(x,y+1)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water above water: " << x << "," << y+1 << std::endl;
#endif
                newLevel.SetState(x,y+1,water);
              }
            }

            if (x>0) {
              if (level.GetState(x-1,y)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water left of water: " << x-1 << "," << y << std::endl;
#endif
                newLevel.SetState(x-1,y,water);
              }
            }

            if (x<level.cellXCount-1) {
              if (level.GetState(x+1,y)==unknown) {
#if defined(DEBUG_TILING)
                std::cout << "Water right of water: " << x+1 << "," << y << std::endl;
#endif
                newLevel.SetState(x+1,y,water);
              }
            }
          }
        }
      }

      level=newLevel;
    }
  }

  bool WaterIndexGenerator::containsCoord(const std::list<GroundTile> &tiles,
                                          const GroundTile::Coord &coord)
  {
    for (const auto &tile:tiles){
      for (const auto &c:tile.coords){
        if (c==coord){
          return true;
        }
      }
    }
    return false;
  }
  
  void WaterIndexGenerator::FillWaterAroundIsland(Progress& progress,
                                                  Level& level,
                                                  std::map<Pixel,std::list<GroundTile>>& cellGroundTileMap)
  {
    progress.Info("Filling water around islands");
    
    for (const auto &entry:cellGroundTileMap){
      Pixel coord=entry.first;

      CellBoundaries cellBoundaries(level,coord);
      bool fillWater=false;

      if (!fillWater && coord.y>0 && level.GetState(coord.x,coord.y-1)==water &&
          (!containsCoord(entry.second, cellBoundaries.borderCoords[0]) ||
           !containsCoord(entry.second, cellBoundaries.borderCoords[1]))){
        fillWater=true;
      }
      if (!fillWater && coord.y<(level.cellYCount-1) && level.GetState(coord.x,coord.y+1)==water &&
          (!containsCoord(entry.second, cellBoundaries.borderCoords[2]) ||
           !containsCoord(entry.second, cellBoundaries.borderCoords[3]))){
        fillWater=true;
      }
      if (!fillWater && coord.x>0 && level.GetState(coord.x-1,coord.y)==water &&
          (!containsCoord(entry.second, cellBoundaries.borderCoords[0]) ||
           !containsCoord(entry.second, cellBoundaries.borderCoords[2]))){
        fillWater=true;
      }
      if (!fillWater && coord.x<(level.cellXCount-1) && level.GetState(coord.x+1,coord.y)==water &&
          (!containsCoord(entry.second, cellBoundaries.borderCoords[1]) ||
           !containsCoord(entry.second, cellBoundaries.borderCoords[3]))){
        fillWater=true;
      }

      if (fillWater){
        GroundTile groundTile(GroundTile::water);
#if defined(DEBUG_TILING)
        std::cout << "Add water base to tile with islands: " << coord.x << "," << coord.y << std::endl;
#endif

        groundTile.coords.push_back(cellBoundaries.borderCoords[0]);
        groundTile.coords.push_back(cellBoundaries.borderCoords[1]);
        groundTile.coords.push_back(cellBoundaries.borderCoords[2]);
        groundTile.coords.push_back(cellBoundaries.borderCoords[3]);

        cellGroundTileMap[coord].push_front(groundTile);
      }
    }
  }

  /**
   * Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
   *
   * Scanning from left to right and bottom to top: Every tile that is unknown
   * but is placed between land and coast or land cells must be land, too.
   */
  void WaterIndexGenerator::FillLand(Progress& progress,
                                     Level& level)
  {
    progress.Info("Filling land");

    bool cont=true;

    while (cont) {
      cont=false;

      // Left to right
      for (uint32_t y=0; y<level.cellYCount; y++) {
        uint32_t x=0;
        uint32_t start=0;
        uint32_t end=0;
        uint32_t state=0;

        while (x<level.cellXCount) {
          switch (state) {
            case 0:
              if (level.GetState(x,y)==land) {
                state=1;
              }
              x++;
              break;
            case 1:
              if (level.GetState(x,y)==unknown) {
                state=2;
                start=x;
                end=x;
                x++;
              }
              else {
                state=0;
              }
              break;
            case 2:
              if (level.GetState(x,y)==unknown) {
                end=x;
                x++;
              }
              else if (level.GetState(x,y)==coast || level.GetState(x,y)==land) {
                if (start<level.cellXCount && end<level.cellXCount && start<=end) {
                  for (uint32_t i=start; i<=end; i++) {
#if defined(DEBUG_TILING)
                    std::cout << "Land between: " << i << "," << y << std::endl;
#endif
                    level.SetState(i,y,land);
                    cont=true;
                  }
                }

                state=0;
              }
              else {
                state=0;
              }
              break;
          }
        }
      }

      //Bottom Up
      for (uint32_t x=0; x<level.cellXCount; x++) {
        uint32_t y=0;
        uint32_t start=0;
        uint32_t end=0;
        uint32_t state=0;

        while (y<level.cellYCount) {
          switch (state) {
            case 0:
              if (level.GetState(x,y)==land) {
                state=1;
              }
              y++;
              break;
            case 1:
              if (level.GetState(x,y)==unknown) {
                state=2;
                start=y;
                end=y;
                y++;
              }
              else {
                state=0;
              }
              break;
            case 2:
              if (level.GetState(x,y)==unknown) {
                end=y;
                y++;
              }
              else if (level.GetState(x,y)==coast || level.GetState(x,y)==land) {
                if (start<level.cellYCount && end<level.cellYCount && start<=end) {
                  for (uint32_t i=start; i<=end; i++) {
#if defined(DEBUG_TILING)
                    std::cout << "Land between: " << x << "," << i << std::endl;
#endif
                    level.SetState(x,i,land);
                    cont=true;
                  }
                }

                state=0;
              }
              else {
                state=0;
              }
              break;
          }
        }
      }
    }
  }

  void WaterIndexGenerator::DumpIndexHeader(const ImportParameter& parameter,
                                            FileWriter& writer,
                                            std::vector<Level>& levels)
  {
    writer.WriteNumber((uint32_t)(parameter.GetWaterIndexMinMag()));
    writer.WriteNumber((uint32_t)(parameter.GetWaterIndexMaxMag()));

    for (auto& level : levels) {
      level.indexEntryOffset=writer.GetPos();
      writer.Write(level.hasCellData);
      writer.Write(level.dataOffsetBytes);
      writer.Write((uint8_t)level.defaultCellData);
      writer.WriteFileOffset(level.indexDataOffset);
      writer.WriteNumber(level.cellXStart);
      writer.WriteNumber(level.cellXEnd);
      writer.WriteNumber(level.cellYStart);
      writer.WriteNumber(level.cellYEnd);
    }
  }

  /**
   * Fills coords information for cells that completely contain a coastline
   *
   * @param progress
   * @param level
   * @param data
   * @param cellGroundTileMap
   */
  void WaterIndexGenerator::HandleAreaCoastlinesCompletelyInACell(Progress& progress,
                                                                  const Level& level,
                                                                  Data& data,
                                                                  std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap)
  {
    progress.Info("Handle area coastline completely in a cell");

    size_t currentCoastline=1;
    for (const auto& coastline : data.coastlines) {
      progress.SetProgress(currentCoastline,data.coastlines.size());

      currentCoastline++;

      if (!(coastline->isArea &&
            coastline->isCompletelyInCell)) {
        continue;
      }

      if (!level.IsInAbsolute(coastline->cell.x,coastline->cell.y)) {
        continue;
      }

      Pixel      coord(coastline->cell.x-level.cellXStart,coastline->cell.y-level.cellYStart);

      GroundTile type=GroundTile::land;
      if (coastline->left==CoastState::unknown)
        type=GroundTile::unknown;
      if (coastline->left==CoastState::water)
        type=GroundTile::water; // should not happen on the Earth
      GroundTile groundTile(type);

      double cellMinLat=level.cellHeight*coastline->cell.y-90.0;
      double cellMinLon=level.cellWidth*coastline->cell.x-180.0;

      groundTile.coords.reserve(coastline->points.size());

      for (size_t p=0; p<coastline->points.size(); p++) {
        groundTile.coords.push_back(Transform(coastline->points[p],level,cellMinLat,cellMinLon,true));
      }

      if (!groundTile.coords.empty()) {
        groundTile.coords.back().coast=false;

#if defined(DEBUG_TILING)
        std::cout << "Coastline in cell: " << coord.x << "," << coord.y << std::endl;
#endif

        cellGroundTileMap[coord].push_back(groundTile);
      }
    }
  }

  static bool IsLeftOnSameBorder(size_t border, const GeoCoord& a,const GeoCoord& b)
  {
    switch (border) {
    case 0:
      return b.GetLon()>=a.GetLon();
    case 1:
      return b.GetLat()<=a.GetLat();
    case 2:
      return b.GetLon()<=a.GetLon();
    case 3:
      return b.GetLat()>=a.GetLat();
    }

    assert(false);

    return false;
  }

  void WaterIndexGenerator::GetCells(const Level& level,
                                     const GeoCoord& a,
                                     const GeoCoord& b,
                                     std::set<Pixel>& cellIntersections)
  {
    uint32_t cx1=(uint32_t)((a.GetLon()+180.0)/level.cellWidth);
    uint32_t cy1=(uint32_t)((a.GetLat()+90.0)/level.cellHeight);

    uint32_t cx2=(uint32_t)((b.GetLon()+180.0)/level.cellWidth);
    uint32_t cy2=(uint32_t)((b.GetLat()+90.0)/level.cellHeight);

    cellIntersections.insert(Pixel(cx1,cy1));

    if (cx1!=cx2 || cy1!=cy2) {
      for (uint32_t x=std::min(cx1,cx2); x<=std::max(cx1,cx2); x++) {
        for (uint32_t y=std::min(cy1,cy2); y<=std::max(cy1,cy2); y++) {

          Pixel    coord(x,y);
          GeoCoord borderPoints[5];
          double   lonMin,lonMax,latMin,latMax;

          lonMin=x*level.cellWidth-180.0;
          lonMax=lonMin+level.cellWidth;
          latMin=y*level.cellHeight-90.0;
          latMax=latMin+level.cellHeight;

          borderPoints[0].Set(latMax,lonMin); // top left
          borderPoints[1].Set(latMax,lonMax); // top right
          borderPoints[2].Set(latMin,lonMax); // bottom right
          borderPoints[3].Set(latMin,lonMin); // bottom left
          borderPoints[4]=borderPoints[0];    // To avoid "% 4" on all indexes

          size_t corner=0;

          while (corner<4) {
            if (LinesIntersect(a,
                               b,
                               borderPoints[corner],
                               borderPoints[corner+1])) {
              cellIntersections.insert(coord);

              break;
            }

            corner++;
          }
        }
      }
    }
  }

  void WaterIndexGenerator::GetCells(const Level& level,
                                     const std::vector<GeoCoord>& points,
                                     std::set<Pixel>& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      GetCells(level,points[p],points[p+1],cellIntersections);
    }
  }

  void WaterIndexGenerator::GetCells(const Level& level,
                                     const std::vector<Point>& points,
                                     std::set<Pixel>& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      GetCells(level,points[p].GetCoord(),points[p+1].GetCoord(),cellIntersections);
    }
  }

  void WaterIndexGenerator::GetCellIntersections(const Level& level,
                                                 const std::vector<GeoCoord>& points,
                                                 size_t coastline,
                                                 std::map<Pixel,std::list<IntersectionRef>>& cellIntersections)
  {
    for (size_t p=0; p<points.size()-1; p++) {
      // Cell coordinates of the current and the next point
      uint32_t cx1=(uint32_t)((points[p].GetLon()+180.0)/level.cellWidth);
      uint32_t cy1=(uint32_t)((points[p].GetLat()+90.0)/level.cellHeight);

      uint32_t cx2=(uint32_t)((points[p+1].GetLon()+180.0)/level.cellWidth);
      uint32_t cy2=(uint32_t)((points[p+1].GetLat()+90.0)/level.cellHeight);

      if (cx1!=cx2 || cy1!=cy2) {
        for (uint32_t x=std::min(cx1,cx2); x<=std::max(cx1,cx2); x++) {
          for (uint32_t y=std::min(cy1,cy2); y<=std::max(cy1,cy2); y++) {

            if (!level.IsInAbsolute(x,y)) {
              continue;
            }

            Pixel    coord(x-level.cellXStart,y-level.cellYStart);
            GeoCoord borderPoints[5];
            double   lonMin,lonMax,latMin,latMax;

            lonMin=x*level.cellWidth-180.0;
            lonMax=(x+1)*level.cellWidth-180.0;
            latMin=y*level.cellHeight-90.0;
            latMax=(y+1)*level.cellHeight-90.0;

            borderPoints[0].Set(latMax,lonMin); // top left
            borderPoints[1].Set(latMax,lonMax); // top right
            borderPoints[2].Set(latMin,lonMax); // bottom right
            borderPoints[3].Set(latMin,lonMin); // bottom left
            borderPoints[4]=borderPoints[0];    // To avoid modula 4 on all indexes

            size_t          intersectionCount=0;
            IntersectionRef firstIntersection=std::make_shared<Intersection>();
            IntersectionRef secondIntersection=std::make_shared<Intersection>();
            size_t          corner=0;

            // Check intersection with one of the borders
            while (corner<4) {
              if (GetLineIntersection(points[p],
                                      points[p+1],
                                      borderPoints[corner],
                                      borderPoints[corner+1],
                                      firstIntersection->point)) {
                intersectionCount++;

                firstIntersection->coastline=coastline;
                firstIntersection->prevWayPointIndex=p;
                firstIntersection->distanceSquare=DistanceSquare(points[p],firstIntersection->point);
                firstIntersection->borderIndex=corner;

                corner++;
                break;
              }

              corner++;
            }

            // Check if there is another intersection with one of the following borders
            while (corner<4) {
              if (GetLineIntersection(points[p],
                                      points[p+1],
                                      borderPoints[corner],
                                      borderPoints[corner+1],
                                      secondIntersection->point)) {
                intersectionCount++;

                secondIntersection->coastline=coastline;
                secondIntersection->prevWayPointIndex=p;
                secondIntersection->distanceSquare=DistanceSquare(points[p],secondIntersection->point);
                secondIntersection->borderIndex=corner;

                corner++;
                break;
              }

              corner++;
            }

            // After above steps we can have 0..2 intersections

            if (x==cx1 &&
                y==cy1) {
              assert(intersectionCount==1 ||
                     intersectionCount==2);

              if (intersectionCount==1) {
                // The segment always leaves the origin cell
                firstIntersection->direction=Direction::out;
                cellIntersections[coord].push_back(firstIntersection);
              }
              else if (intersectionCount==2) {
                // If we have two intersections with borders of cells between the starting cell and the
                // target cell then the one closer to the starting point is the incoming and the one farer
                // away is the leaving one
                double firstLength=DistanceSquare(firstIntersection->point,points[p]);
                double secondLength=DistanceSquare(secondIntersection->point,points[p]);

                if (firstLength<=secondLength) {
                  firstIntersection->direction=Direction::in; // Enter
                  cellIntersections[coord].push_back(firstIntersection);

                  secondIntersection->direction=Direction::out; // Leave
                  cellIntersections[coord].push_back(secondIntersection);
                }
                else {
                  secondIntersection->direction=Direction::in; // Enter
                  cellIntersections[coord].push_back(secondIntersection);

                  firstIntersection->direction=Direction::out; // Leave
                  cellIntersections[coord].push_back(firstIntersection);

                }
              }
            }
            else if (x==cx2 &&
                     y==cy2) {
              assert(intersectionCount==1 ||
                     intersectionCount==2);

              if (intersectionCount==1) {
                // The segment always enters the target cell
                firstIntersection->direction=Direction::in;
                cellIntersections[coord].push_back(firstIntersection);
              }
              else if (intersectionCount==2) {
                // If we have two intersections with borders of cells between the starting cell and the
                // target cell then the one closer to the starting point is the incoming and the one farer
                // away is the leaving one
                double firstLength=DistanceSquare(firstIntersection->point,points[p]);
                double secondLength=DistanceSquare(secondIntersection->point,points[p]);

                if (firstLength<=secondLength) {
                  firstIntersection->direction=Direction::in; // Enter
                  cellIntersections[coord].push_back(firstIntersection);

                  secondIntersection->direction=Direction::out; // Leave
                  cellIntersections[coord].push_back(secondIntersection);
                }
                else {
                  secondIntersection->direction=Direction::in; // Enter
                  cellIntersections[coord].push_back(secondIntersection);

                  firstIntersection->direction=Direction::out; // Leave
                  cellIntersections[coord].push_back(firstIntersection);

                }
              }
            }
            else {
              assert(intersectionCount==0 ||
                     intersectionCount==1 ||
                     intersectionCount==2);

              if (intersectionCount==1) {
                // If we only have one intersection with borders of cells between the starting borderPoints and the
                // target borderPoints then this is a "touch"
                firstIntersection->direction=Direction::touch;
                cellIntersections[coord].push_back(firstIntersection);
              }
              else if (intersectionCount==2) {
                // If we have two intersections with borders of cells between the starting cell and the
                // target cell then the one closer to the starting point is the incoming and the one farer
                // away is the leaving one
                double firstLength=DistanceSquare(firstIntersection->point,points[p]);
                double secondLength=DistanceSquare(secondIntersection->point,points[p]);

                if (firstLength<=secondLength) {
                  firstIntersection->direction=Direction::in; // Enter
                  cellIntersections[coord].push_back(firstIntersection);

                  secondIntersection->direction=Direction::out; // Leave
                  cellIntersections[coord].push_back(secondIntersection);
                }
                else {
                  secondIntersection->direction=Direction::in; // Enter
                  cellIntersections[coord].push_back(secondIntersection);

                  firstIntersection->direction=Direction::out; // Leave
                  cellIntersections[coord].push_back(firstIntersection);

                }
              }
            }
          }
        }
      }
    }
  }

  /**
   * Collects, calculates and generates a number of data about a coastline.
   */
  void WaterIndexGenerator::GetCoastlineData(const ImportParameter& parameter,
                                             Progress& progress,
                                             const Projection& projection,
                                             const Level& level,
                                             const std::list<CoastRef>& coastlines,
                                             Data& data)
  {
    progress.Info("Calculate coastline data");

    data.coastlines.resize(coastlines.size());

    size_t curCoast=0;
    for (const auto& coast : coastlines) {
      CoastlineDataRef coastline=std::make_shared<CoastlineData>();
      data.coastlines[curCoast]=coastline;
      progress.SetProgress(curCoast,coastlines.size());

      TransPolygon polygon;

      if (coast->isArea) {
        polygon.TransformArea(projection,
                              parameter.GetOptimizationWayMethod(),
                              coast->coast,
                              1.0,
                              TransPolygon::simple);
      }
      else {
        polygon.TransformWay(projection,
                             parameter.GetOptimizationWayMethod(),
                             coast->coast,
                             1.0,
                             TransPolygon::simple);
      }

      if (coast->isArea) {
        double minX=polygon.points[polygon.GetStart()].x;
        double minY=polygon.points[polygon.GetStart()].y;
        double maxX=minX;
        double maxY=minY;

        for (size_t p=polygon.GetStart()+1; p<=polygon.GetEnd(); p++) {
          if (polygon.points[p].draw) {
            minX=std::min(minX,polygon.points[p].x);
            maxX=std::max(maxX,polygon.points[p].x);
            minY=std::min(minY,polygon.points[p].y);
            maxY=std::max(maxY,polygon.points[p].y);
          }
        }

        double pixelWidth=maxX-minX;
        double pixelHeight=maxY-minY;

        // Artificial values but for drawing an area a box of at least 4x4 might make sense
        if (pixelWidth<=4.0 ||
            pixelHeight<=4.0) {
          continue;
        }
      }

      coastline->id=coast->id;
      coastline->isArea=coast->isArea;
      coastline->right=coast->right;
      coastline->left=coast->left;

      coastline->points.reserve(polygon.GetLength());
      for (size_t p=polygon.GetStart(); p<=polygon.GetEnd(); p++) {
        if (polygon.points[p].draw) {
          coastline->points.push_back(GeoCoord(coast->coast[p].GetLat(),
                                                              coast->coast[p].GetLon()));
        }
      }

      // Currently transformation optimization code sometimes does not correctly handle the closing point for areas
      if (coast->isArea) {
        if (coastline->points.front()!=coastline->points.back()) {
          coastline->points.push_back(coastline->points.front());
        }
      }

      GeoBox  boundingBox;

      GetBoundingBox(coast->coast,
                     boundingBox);

      uint32_t cxMin,cxMax,cyMin,cyMax;

      cxMin=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/level.cellWidth);
      cxMax=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/level.cellWidth);
      cyMin=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/level.cellHeight);
      cyMax=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/level.cellHeight);

      if (cxMin==cxMax &&
          cyMin==cyMax) {
        coastline->cell.x=cxMin;
        coastline->cell.y=cyMin;
        coastline->isCompletelyInCell=true;
      }
      else {
        coastline->isCompletelyInCell=false;
      }

      if (!coastline->isCompletelyInCell) {
        // Calculate all intersections for all path steps for all cells covered
        GetCellIntersections(level,
                             coastline->points,
                             curCoast,
                             coastline->cellIntersections);

        for (const auto& intersectionEntry : coastline->cellIntersections) {
          data.cellCoastlines[intersectionEntry.first].push_back(curCoast);
        }
      }

      curCoast++;
    }

    // Fix the vector size to remove unused slots (because of filtering by min area size)
    data.coastlines.resize(curCoast);
  }

  WaterIndexGenerator::IntersectionRef WaterIndexGenerator::GetPreviousIntersection(std::list<IntersectionRef>& intersectionsPathOrder,
                                                                                    const IntersectionRef& current)
  {
    std::list<IntersectionRef>::iterator currentIter=intersectionsPathOrder.begin();

    while (currentIter!=intersectionsPathOrder.end() &&
           (*currentIter)!=current) {
      ++currentIter;
    }

    if (currentIter==intersectionsPathOrder.end()) {
      return NULL;
    }

    if (currentIter==intersectionsPathOrder.begin()) {
      return NULL;
    }

    currentIter--;

    return *currentIter;
  }

  /**
   * Closes the sling from the incoming intersection to the outgoing intersection traveling clock wise around the cell
   * border.
   */
  void WaterIndexGenerator::WalkBorderCW(GroundTile& groundTile,
                                         const Level& level,
                                         double cellMinLat,
                                         double cellMinLon,
                                         const IntersectionRef& incoming,
                                         const IntersectionRef& outgoing,
                                         const GroundTile::Coord borderCoords[])
  {

    if (outgoing->borderIndex!=incoming->borderIndex ||
        !IsLeftOnSameBorder(incoming->borderIndex,incoming->point,outgoing->point)) {
      size_t borderPoint=(incoming->borderIndex+1)%4;
      size_t endBorderPoint=outgoing->borderIndex;

      while (borderPoint!=endBorderPoint) {
        groundTile.coords.push_back(borderCoords[borderPoint]);

        if (borderPoint==3) {
          borderPoint=0;
        }
        else {
          borderPoint++;
        }
      }

      groundTile.coords.push_back(borderCoords[borderPoint]);
    }

    groundTile.coords.push_back(Transform(outgoing->point,level,cellMinLat,cellMinLon,false));
  }


  WaterIndexGenerator::IntersectionRef WaterIndexGenerator::GetNextCW(const std::list<IntersectionRef>& intersectionsCW,
                                                                      const IntersectionRef& current) const
  {
    std::list<IntersectionRef>::const_iterator next=intersectionsCW.begin();

    while (next!=intersectionsCW.end() &&
           (*next)!=current) {
      next++;
    }

    assert(next!=intersectionsCW.end());

    next++;

    if (next==intersectionsCW.end()) {
      next=intersectionsCW.begin();
    }

    assert(next!=intersectionsCW.end());

    return *next;
  }

  void WaterIndexGenerator::WalkPathBack(GroundTile& groundTile,
                                         const Level& level,
                                         double cellMinLat,
                                         double cellMinLon,
                                         const IntersectionRef& pathStart,
                                         const IntersectionRef& pathEnd,
                                         const std::vector<GeoCoord>& points,
                                         bool isArea)
  {
    groundTile.coords.back().coast=true;

    if (isArea) {
      if (pathStart->prevWayPointIndex==pathEnd->prevWayPointIndex &&
          pathStart->distanceSquare>pathEnd->distanceSquare) {
        groundTile.coords.push_back(Transform(pathEnd->point,level,cellMinLat,cellMinLon,false));
      }
      else {
        size_t idx=pathStart->prevWayPointIndex;
        size_t targetIdx=pathEnd->prevWayPointIndex+1;

        if (targetIdx==points.size()) {
          targetIdx=0;
        }

        while (idx!=targetIdx) {
          groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));

          if (idx>0) {
            idx--;
          }
          else {
            idx=points.size()-1;
          }
        }

        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));

        groundTile.coords.push_back(Transform(pathEnd->point,level,cellMinLat,cellMinLon,false));
      }
    }
    else {
      size_t targetIdx=pathEnd->prevWayPointIndex+1;

      for (size_t idx=pathStart->prevWayPointIndex;
          idx>=targetIdx;
          idx--) {
        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));
      }

      groundTile.coords.push_back(Transform(pathEnd->point,level,cellMinLat,cellMinLon,false));
    }
  }

  void WaterIndexGenerator::WalkPathForward(GroundTile& groundTile,
                                            const Level& level,
                                            double cellMinLat,
                                            double cellMinLon,
                                            const IntersectionRef& pathStart,
                                            const IntersectionRef& pathEnd,
                                            const std::vector<GeoCoord>& points,
                                            bool isArea)
  {
    groundTile.coords.back().coast=true;

    if (isArea) {
      if (pathStart->prevWayPointIndex==pathEnd->prevWayPointIndex &&
          pathStart->distanceSquare<pathEnd->distanceSquare) {
        groundTile.coords.push_back(Transform(pathEnd->point,level,cellMinLat,cellMinLon,false));
      }
      else {
        size_t idx=pathStart->prevWayPointIndex+1;
        size_t targetIdx=pathEnd->prevWayPointIndex;

        if (targetIdx==points.size()) {
          targetIdx=0;
        }

        while (idx!=targetIdx) {
          groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));

          if (idx>=points.size()-1) {
            idx=0;
          }
          else {
            idx++;
          }
        }

        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));

        groundTile.coords.push_back(Transform(pathEnd->point,level,cellMinLat,cellMinLon,false));
      }
    }
    else {
      size_t targetIdx=pathEnd->prevWayPointIndex;

      for (size_t idx=pathStart->prevWayPointIndex+1;
          idx<=targetIdx;
          idx++) {
        groundTile.coords.push_back(Transform(points[idx],level,cellMinLat,cellMinLon,true));
      }

      groundTile.coords.push_back(Transform(pathEnd->point,level,cellMinLat,cellMinLon,false));
    }
  }

  /**
   * If intersection->direction==Direction::in, we are searching next OUT intersection,
   * previos IN intersection othervise.
   */
  WaterIndexGenerator::IntersectionRef WaterIndexGenerator::FindSiblingIntersection(const IntersectionRef &intersection,
                                                                                    const std::list<IntersectionRef> &intersectionsCW,
                                                                                    bool isArea)
  {
    Direction searchDirection=intersection->direction==Direction::in ? Direction::out : Direction::in;
    std::list<IntersectionRef> candidates;
    for (auto const &i:intersectionsCW){
      if (intersection->coastline==i->coastline && i->direction==searchDirection){
        candidates.push_back(i);
      }
    }

    IntersectionRef result;
    for (auto const &i:candidates){
      if (intersection->direction==Direction::in){
        if (i->prevWayPointIndex >= intersection->prevWayPointIndex){
          if ((!result) || (result && i->prevWayPointIndex < result->prevWayPointIndex)){
            result=i;
          }
        }
      }else{
        if (i->prevWayPointIndex <= intersection->prevWayPointIndex){
          if ((!result) || (result && i->prevWayPointIndex > result->prevWayPointIndex)){
            result=i;
          }
        }
      }
    }
    if (result || !isArea){
      return result;
    }
    for (auto const &i:candidates){
      if (intersection->direction==Direction::in){
        if (i->prevWayPointIndex <= intersection->prevWayPointIndex){
          if ((!result) || (result && i->prevWayPointIndex < result->prevWayPointIndex)){
            result=i;
          }
        }
      }else{
        if (i->prevWayPointIndex >= intersection->prevWayPointIndex){
          if ((!result) || (result && i->prevWayPointIndex > result->prevWayPointIndex)){
            result=i;
          }
        }
      }
    }
    return result;
  }

  bool WaterIndexGenerator::FindTripoint(const IntersectionRef pathStart,
                                         IntersectionRef &pathEnd,
                                         IntersectionRef &startNext,
                                         IntersectionRef &endNext,
                                         Data &data,
                                         const std::list<IntersectionRef> intersectionsCW
                                         )
  {
    CoastlineDataRef coastline=data.coastlines[pathStart->coastline];
    GeoCoord tripoint=(pathStart->direction==Direction::in) ? coastline->points.back() : coastline->points.front();

    // get index of pathStart in intersectionsCW
    int startIndex=-1;
    for (const auto &intersection:intersectionsCW){
      startIndex++;
      if (intersection==pathStart){
        break;
      }
    }

    // try to find coastline starting/ending in tripoint...
    int i=-1;
    int nextIndex=-1;
    int nextDistance=0;
    for (const auto &intersection:intersectionsCW){
      i++;
      CoastlineDataRef c=data.coastlines[intersection->coastline];
      if (intersection->coastline==pathStart->coastline || c->isArea){
        continue;
      }
      GeoCoord coord=(intersection->direction==Direction::in) ? c->points.back() : c->points.front();
      if (tripoint!=coord){
        continue;
      }
      // ...that is nearest CCW to pathStart
      int distance=(i>startIndex) ? intersectionsCW.size()-(i-startIndex) : (startIndex-i);
      if (nextIndex==-1 || distance<nextDistance){
        endNext=intersection;
        nextIndex=i;
        nextDistance=distance;
      }
    }
    if (nextIndex<0){
      return false;
    }

    // try to find nearest intersection on found coastline to tripoint
    for (const auto &intersection:intersectionsCW){
      if (intersection->coastline==endNext->coastline && intersection->direction==endNext->direction){
        if (endNext->direction==Direction::in){
          if (intersection->prevWayPointIndex > endNext->prevWayPointIndex){
            endNext=intersection;
          }
        }else{
          if (intersection->prevWayPointIndex < endNext->prevWayPointIndex){
            endNext=intersection;
          }
        }
      }
    }

    // create synthetic pathEnd, startNext
    pathEnd=std::make_shared<Intersection>();
    pathEnd->coastline=pathStart->coastline;
    pathEnd->prevWayPointIndex=(pathStart->direction==Direction::in) ? coastline->points.size()-1 : 0;
    pathEnd->point=tripoint;
    pathEnd->distanceSquare=0; // ?
    pathEnd->direction=(pathStart->direction==Direction::in) ? Direction::out : Direction::in;
    pathEnd->borderIndex=0; // ?

    coastline=data.coastlines[endNext->coastline];
    startNext=std::make_shared<Intersection>();
    startNext->coastline=endNext->coastline;
    startNext->prevWayPointIndex=(endNext->direction==Direction::in) ? coastline->points.size()-1 : 0;
    startNext->point=tripoint;
    startNext->distanceSquare=0; // ?
    startNext->direction=(endNext->direction==Direction::in) ? Direction::out : Direction::in;
    startNext->borderIndex=0; // ?

    return true;
  }

  void WaterIndexGenerator::WalkPath(GroundTile &groundTile,
                                     const Level& level,
                                     const CellBoundaries &cellBoundaries,
                                     const IntersectionRef pathStart,
                                     const IntersectionRef pathEnd,
                                     CoastlineDataRef coastline)
  {
#if defined(DEBUG_COASTLINE)
      std::cout << "     ... path from " << pathStart->point.GetDisplayText() <<
                                  " to " << pathEnd->point.GetDisplayText() << std::endl;
#endif
      if (pathStart->direction==Direction::out){
        WalkPathBack(groundTile,
                     level,
                     cellBoundaries.latMin,
                     cellBoundaries.lonMin,
                     pathStart,
                     pathEnd,
                     coastline->points,
                     coastline->isArea);
      }else{
        WalkPathForward(groundTile,
                        level,
                        cellBoundaries.latMin,
                        cellBoundaries.lonMin,
                        pathStart,
                        pathEnd,
                        coastline->points,
                        coastline->isArea);
      }
  }

  bool WaterIndexGenerator::WalkBoundaryCW(GroundTile &groundTile,
                                           const Level& level,
                                           const IntersectionRef startIntersection,
                                           const std::list<IntersectionRef> &intersectionsCW,
                                           std::set<IntersectionRef> &visitedIntersections,
                                           const CellBoundaries &cellBoundaries,
                                           Data& data)
  {
#if defined(DEBUG_COASTLINE)
      std::cout << "   walk around " << TypeToString(groundTile.type) <<
        " from " << startIntersection->point.GetDisplayText() << std::endl;
#endif

    groundTile.coords.push_back(Transform(startIntersection->point,level,cellBoundaries.latMin,cellBoundaries.lonMin,false));

    IntersectionRef pathStart=startIntersection;
    bool error=false;
    size_t step=0;
    while ((step==0 || pathStart!=startIntersection) && !error){
      step++;
      visitedIntersections.insert(pathStart);
      CoastlineDataRef coastline=data.coastlines[pathStart->coastline]; // TODO: check that we have correct type
      IntersectionRef pathEnd=FindSiblingIntersection(pathStart,
                                                      intersectionsCW,
                                                      coastline->isArea);
      if (!pathEnd){
#if defined(DEBUG_COASTLINE)
        std::cout << "     can't found sibling intersection for " << pathStart->point.GetDisplayText() << std::endl;
#endif

        // handle coastline Tripoint
        IntersectionRef startNext;
        IntersectionRef endNext;
        if (coastline->isArea ||
            !FindTripoint(pathStart,
                          pathEnd,
                          startNext,
                          endNext,
                          data,
                          intersectionsCW
                          )){
          return false;
        }
#if defined(DEBUG_COASTLINE)
        std::cout << "     found tripoint " << pathEnd->point.GetDisplayText() << std::endl;
#endif

        WalkPath(groundTile, level, cellBoundaries, pathStart, pathEnd, coastline);
        pathStart=startNext;
        pathEnd=endNext;
        coastline=data.coastlines[pathStart->coastline];
      }

      if (step>1000){
        // put breakpoint here if coputation stucks in this loop :-/
        std::cout << "   too many steps, give up... " << step << std::endl;
        return false;
      }

      WalkPath(groundTile, level, cellBoundaries, pathStart, pathEnd, coastline);
      
      pathStart=GetNextCW(intersectionsCW,
                          pathEnd);

      WalkBorderCW(groundTile,
                   level,
                   cellBoundaries.latMin,
                   cellBoundaries.lonMin,
                   pathEnd,
                   pathStart,
                   cellBoundaries.borderCoords);
      
    }


    return true;
  }

  void WaterIndexGenerator::HandleCoastlineCell(const Pixel &cell,
                                                const std::list<size_t>& intersectCoastlines,
                                                const Level& level,
                                                std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                                                Data& data)
  {
      std::list<IntersectionRef>    intersectionsCW;        // Intersections in clock wise order over all coastlines
      std::set<IntersectionRef>     visitedIntersections;
      CellBoundaries                cellBoundaries(level,cell);
   
      // For every coastline by index intersecting the current cell
      for (const auto& currentCoastline : intersectCoastlines) {
        CoastlineDataRef coastData=data.coastlines[currentCoastline];
        std::map<Pixel,std::list<IntersectionRef>>::iterator cellData=coastData->cellIntersections.find(cell);
        assert(cellData!=coastData->cellIntersections.end());
        intersectionsCW.insert(intersectionsCW.end(), cellData->second.begin(), cellData->second.end());
      }
      intersectionsCW.sort(IntersectionCWComparator());

#if defined(DEBUG_COASTLINE)
      std::cout.precision(5);
      std::cout << "    cell boundaries" <<
        ": " << cellBoundaries.latMin << " " << cellBoundaries.lonMin <<
        "; " << cellBoundaries.latMin << " " << cellBoundaries.lonMax <<
        "; " << cellBoundaries.latMax << " " << cellBoundaries.lonMin <<
        "; " << cellBoundaries.latMax << " " << cellBoundaries.lonMax <<
        std::endl;
      std::cout << "    intersections:" << std::endl;
      for (const auto &intersection: intersectionsCW){
        std::cout << "      " << intersection->point.GetDisplayText() << " (" << intersection->coastline << ", ";
        std::cout << (intersection->direction==Direction::out ? "out" : "in");
        std::cout << ", " << intersection->prevWayPointIndex;
        std::cout << ")" << std::endl;
      }
#endif

      for (const auto &intersection: intersectionsCW){
        if (intersection->direction==Direction::touch){
          continue; // TODO: what to do?
        }
        if (visitedIntersections.find(intersection)!=visitedIntersections.end()){
          continue;
        }
        CoastlineDataRef coastline=data.coastlines[intersection->coastline];

        CoastState coastState=intersection->direction==Direction::in?coastline->right:coastline->left;
        assert(coastState!=CoastState::undefined);
        GroundTile groundTile(GroundTile::Type::unknown);
        if (coastState==CoastState::land)
          groundTile.type=GroundTile::Type::land;
        if (coastState==CoastState::water)
          groundTile.type=GroundTile::Type::water;

        if (!WalkBoundaryCW(groundTile,
                            level,
                            intersection,
                            intersectionsCW,
                            visitedIntersections,
                            cellBoundaries,
                            data)){
            std::cout << "Can't walk around cell boundary!" << std::endl;
            continue;
        }

        cellGroundTileMap[cell].push_back(groundTile);
      }
  }

  /**
   * Fills coords information for cells that intersect a coastline
   */
  void WaterIndexGenerator::HandleCoastlinesPartiallyInACell(Progress& progress,
                                                             const Level& level,
                                                             std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                                                             Data& data)
  {
    progress.Info("Handle coastlines partially in a cell");

    // For every cell with intersections
    size_t currentCell=0;
    for (const auto& cellEntry : data.cellCoastlines) {
      progress.SetProgress(currentCell,data.cellCoastlines.size());
      currentCell++;

#if defined(DEBUG_COASTLINE)
      std::cout << " - cell " << cellEntry.first.x << " " << cellEntry.first.y <<
        " (level offset " << level.indexEntryOffset << "): " << std::endl;
#endif

      HandleCoastlineCell(cellEntry.first,
                          cellEntry.second,
                          level,
                          cellGroundTileMap,
                          data);
    }
  }

  void WaterIndexGenerator::buildTiles(const TypeConfigRef &typeConfig,
                                       const ImportParameter &parameter,
                                       Progress &progress,
                                       const MercatorProjection &projection,
                                       Level &levelStruct,
                                       std::map<Pixel,std::list<GroundTile>> &cellGroundTileMap,
                                       const std::list<CoastRef> &coastlines,
                                       Data &data,
                                       const std::list<CoastRef>& dataPolygon)
  {
    if (!coastlines.empty()) {
      // Collects, calculates and generates a number of data about a coastline
      GetCoastlineData(parameter,
                       progress,
                       projection,
                       levelStruct,
                       coastlines,
                       data);

      // Mark cells that intersect a coastline as coast
      MarkCoastlineCells(progress,
                         levelStruct,
                         data);

      // Fills coords information for cells that intersect a coastline
      HandleCoastlinesPartiallyInACell(progress,
                                       levelStruct,
                                       cellGroundTileMap,
                                       data);

      // Fills coords information for cells that completely contain a coastline
      HandleAreaCoastlinesCompletelyInACell(progress,
                                            levelStruct,
                                            data,
                                            cellGroundTileMap);
    }

    // Calculate the cell type for cells directly around coast cells
    CalculateCoastEnvironment(progress,
                              levelStruct,
                              cellGroundTileMap);

    if (parameter.GetAssumeLand()) {
      // Assume cell type 'land' for cells that intersect with 'land' object types
      AssumeLand(parameter,
                 progress,
                 *typeConfig,
                 levelStruct);
    }

    if (!coastlines.empty()) {
      // Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
      FillWater(progress,
                levelStruct,
                /*tileCount*/10,
                dataPolygon);

      FillWaterAroundIsland(progress, levelStruct, cellGroundTileMap);
    }

    // Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
    FillLand(progress,
             levelStruct);

    levelStruct.hasCellData=false;
    levelStruct.defaultCellData=unknown;

    if (levelStruct.cellXCount>0 && levelStruct.cellYCount>0) {
      levelStruct.defaultCellData=levelStruct.GetState(0,0);

      if (cellGroundTileMap.size()>0) {
        levelStruct.hasCellData=true;
      }
      else {
        for (uint32_t y=0; y<levelStruct.cellYCount; y++) {
          for (uint32_t x=0; x<levelStruct.cellXCount; x++) {
            levelStruct.hasCellData=levelStruct.GetState(x,y)!=levelStruct.defaultCellData;

            if (levelStruct.hasCellData) {
              break;
            }
          }

          if (levelStruct.hasCellData) {
            break;
          }
        }
      }
    }
  }

  void WaterIndexGenerator::writeTiles(Progress &progress,
                                       const std::map<Pixel,std::list<GroundTile>> &cellGroundTileMap,
                                       const uint32_t level,
                                       Level &levelStruct,
                                       FileWriter &writer)
  {
    if (levelStruct.hasCellData) {

      //
      // Calculate size of data
      //

      size_t dataSize=4;
      char   buffer[10];

      for (const auto& coord : cellGroundTileMap) {
        // Number of ground tiles
        dataSize+=EncodeNumber(coord.second.size(),buffer);

        for (const auto& tile : coord.second) {
          // Type
          dataSize++;

          // Number of coordinates
          dataSize+=EncodeNumber(tile.coords.size(),buffer);

          // Data for coordinate pairs
          dataSize+=tile.coords.size()*2*sizeof(uint16_t);
        }
      }

      levelStruct.dataOffsetBytes=BytesNeededToEncodeNumber(dataSize);

      progress.Info("Writing index for level "+
                    NumberToString(level)+", "+
                    NumberToString(levelStruct.cellXCount*levelStruct.cellXCount)+" cells, "+
                    NumberToString(cellGroundTileMap.size())+" entries, "+
                    NumberToString(levelStruct.dataOffsetBytes)+" bytes/entry, "+
                    ByteSizeToString(1.0*levelStruct.cellXCount*levelStruct.cellYCount*levelStruct.dataOffsetBytes+dataSize));

      //
      // Write bitmap
      //

      levelStruct.indexDataOffset=writer.GetPos();

      for (uint32_t y=0; y<levelStruct.cellYCount; y++) {
        for (uint32_t x=0; x<levelStruct.cellXCount; x++) {
          State state=levelStruct.GetState(x,y);

          writer.WriteFileOffset((FileOffset) state,
                                 levelStruct.dataOffsetBytes);
        }
      }

      //
      // Write data
      //

      FileOffset dataOffset=writer.GetPos();

      // TODO: when data format will be changing, consider usage ones (0xFF..FF) as empty placeholder
      writer.WriteFileOffset((FileOffset)0,4);

      for (const auto& coord : cellGroundTileMap) {
        FileOffset startPos=writer.GetPos();

        writer.WriteNumber((uint32_t) coord.second.size());

        for (const auto& tile : coord.second) {
          writer.Write((uint8_t) tile.type);

          writer.WriteNumber((uint32_t) tile.coords.size());

          for (size_t c=0; c<tile.coords.size(); c++) {
            if (tile.coords[c].coast) {
              uint16_t x=tile.coords[c].x | uint16_t(1 << 15);

              writer.Write(x);
            }
            else {
              writer.Write(tile.coords[c].x);
            }
            writer.Write(tile.coords[c].y);
          }
        }

        FileOffset endPos;
        uint32_t   cellId=coord.first.y*levelStruct.cellXCount+coord.first.x;
        size_t     index =cellId*levelStruct.dataOffsetBytes;

        endPos=writer.GetPos();

        writer.SetPos(levelStruct.indexDataOffset+index);
        writer.WriteFileOffset(startPos-dataOffset,
                               levelStruct.dataOffsetBytes);
        writer.SetPos(endPos);
      }
    }
    else {
      progress.Info("All cells have state '"+StateToString(levelStruct.defaultCellData)+"' and no coastlines, no cell index needed");
    }

    FileOffset currentPos=writer.GetPos();

    writer.SetPos(levelStruct.indexEntryOffset);
    writer.Write(levelStruct.hasCellData);
    writer.Write(levelStruct.dataOffsetBytes);
    writer.Write((uint8_t) levelStruct.defaultCellData);
    writer.WriteFileOffset(levelStruct.indexDataOffset);
    writer.SetPos(currentPos);
  }

  void WaterIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                              ImportModuleDescription& description) const
  {
    description.SetName("WaterIndexGenerator");
    description.SetDescription("Create index for lookup of ground/see tiles");

    description.AddRequiredFile(BoundingBoxDataFile::BOUNDINGBOX_DAT);

    description.AddRequiredFile(Preprocess::RAWCOASTLINE_DAT);

    description.AddRequiredFile(Preprocess::RAWDATAPOLYGON_DAT);

    description.AddRequiredFile(CoordDataFile::COORD_DAT);

    description.AddRequiredFile(WayDataFile::WAYS_DAT);

    description.AddProvidedFile(WaterIndex::WATER_IDX);
  }

  bool WaterIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    std::list<CoastRef> coastlines;
    std::list<CoastRef> dataPolygon;

    FileScanner         scanner;

    GeoBox              boundingBox;
    GeoCoord            minCoord;
    GeoCoord            maxCoord;

    std::vector<Level>  levels;

    // Calculate size of tile cells for the maximum zoom level
    double              cellWidth;
    double              cellHeight;

    //
    // Read bounding box
    //

    BoundingBoxDataFile boundingBoxDataFile;

    if (!boundingBoxDataFile.Load(parameter.GetDestinationDirectory())) {
      progress.Error("Error loading file '"+boundingBoxDataFile.GetFilename()+"'");

      return false;
    }

    boundingBox=boundingBoxDataFile.GetBoundingBox();
    minCoord=boundingBox.GetMinCoord();
    maxCoord=boundingBox.GetMaxCoord();

    //
    // Initialize levels
    //

    levels.resize(parameter.GetWaterIndexMaxMag()-parameter.GetWaterIndexMinMag()+1);

    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t level=0; level<=parameter.GetWaterIndexMaxMag(); level++) {
      if (level>=parameter.GetWaterIndexMinMag() &&
          level<=parameter.GetWaterIndexMaxMag()) {
        levels[level-parameter.GetWaterIndexMinMag()].SetBox(minCoord,
                                                             maxCoord,
                                                             cellWidth,cellHeight);
      }

      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    //
    // Loat data polygon
    //

    if (!LoadDataPolygon(parameter,
                         progress,
                         dataPolygon)) {
      return false;
    }

    //
    // Load and merge coastlines
    //

    if (!LoadCoastlines(parameter,
                        progress,
                        coastlines)) {
      return false;
    }

    MergeCoastlines(progress,coastlines);

    SynthetizeCoastlines(progress,
                         coastlines,
                         dataPolygon);

    progress.SetAction("Writing 'water.idx'");

    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  WaterIndex::WATER_IDX));

      DumpIndexHeader(parameter,
                      writer,
                      levels);

      for (size_t levelIndex=0; levelIndex<levels.size(); levelIndex++) {
        Magnification                          magnification;
        MercatorProjection                     projection;
        Data                                   data;
        std::map<Pixel,std::list<GroundTile> > cellGroundTileMap;

        uint32_t level=levelIndex+parameter.GetWaterIndexMinMag();
        Level &levelStruct=levels[levelIndex];

        magnification.SetLevel(level);

        projection.Set(GeoCoord(0.0,0.0),magnification,72,640,480);

        progress.SetAction("Building tiles for level "+NumberToString(level));

        buildTiles(typeConfig,
                   parameter,
                   progress,
                   projection,
                   levelStruct,
                   cellGroundTileMap,
                   coastlines,
                   data,
                   dataPolygon);

        writeTiles(progress,
                   cellGroundTileMap,
                   level,
                   levelStruct,
                   writer);
      }

      coastlines.clear();

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
