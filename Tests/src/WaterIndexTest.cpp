/*
  WaterIndex - a test program for libosmscout
  Copyright (C) 2022  Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cmath>
#include <cstdlib>
#include <iostream>

#include <osmscoutimport/WaterIndexProcessor.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

TEST_CASE("Merge empty vector of coastlines")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;
  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.empty());
}

WaterIndexProcessor::CoastRef MkCoastline(OSMId id,
                                          std::vector<Point>&& coords,
                                          WaterIndexProcessor::CoastState left=WaterIndexProcessor::CoastState::land,
                                          WaterIndexProcessor::CoastState right=WaterIndexProcessor::CoastState::water)
{
  if (coords.empty()) {
    return std::make_shared<WaterIndexProcessor::Coast>(WaterIndexProcessor::Coast{id, false, 0, 0, coords, right, left});
  }
  bool isArea=coords.front().IsIdentical(coords.back());
  Id frontNodeId = coords.front().GetId();
  Id backNodeId = coords.back().GetId();
  return std::make_shared<WaterIndexProcessor::Coast>(WaterIndexProcessor::Coast{id, isArea, frontNodeId, backNodeId, coords, left, right});
}

TEST_CASE("Merge of coastlines should throw out empty one")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  std::vector<Point> coords;
  coastlines.push_back(MkCoastline(0, std::move(coords)));

  coords.clear();
  coords.emplace_back(0, GeoCoord(0,0));
  coords.emplace_back(0, GeoCoord(1,1));
  coastlines.push_back(MkCoastline(1, std::move(coords)));

  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.size()==1);
  REQUIRE(coastlines.front()->coast.size()==2);
}

TEST_CASE("Merge follow-up coastlines")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  std::vector<Point> coords;
  coords.emplace_back(0, GeoCoord(0,0));
  coords.emplace_back(0, GeoCoord(1,1));
  coastlines.push_back(MkCoastline(0, std::move(coords)));

  coords.clear();
  coords.emplace_back(0, GeoCoord(1,1));
  coords.emplace_back(0, GeoCoord(1,2));
  coastlines.push_back(MkCoastline(1, std::move(coords)));

  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.size()==1);
  REQUIRE(coastlines.front()->coast.size()==3);
  REQUIRE_FALSE(coastlines.front()->isArea);
}

TEST_CASE("Merge coastlines to area")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  std::vector<Point> coords;
  coords.emplace_back(0, GeoCoord(0,0));
  coords.emplace_back(0, GeoCoord(1,1));
  coastlines.push_back(MkCoastline(0, std::move(coords)));

  coords.clear();
  coords.emplace_back(0, GeoCoord(1,1));
  coords.emplace_back(0, GeoCoord(1,2));
  coords.emplace_back(0, GeoCoord(0,0));
  coastlines.push_back(MkCoastline(1, std::move(coords)));

  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.size()==1);
  REQUIRE(coastlines.front()->coast.size()==3);
  REQUIRE(coastlines.front()->isArea);
}

TEST_CASE("Merge with different states")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  std::vector<Point> coords;
  coords.emplace_back(0, GeoCoord(0,0));
  coords.emplace_back(0, GeoCoord(1,1));
  coastlines.push_back(MkCoastline(0, std::move(coords), WaterIndexProcessor::CoastState::land, WaterIndexProcessor::CoastState::water));

  coords.clear();
  coords.emplace_back(0, GeoCoord(1,1));
  coords.emplace_back(0, GeoCoord(1,2));
  coastlines.push_back(MkCoastline(1, std::move(coords), WaterIndexProcessor::CoastState::unknown, WaterIndexProcessor::CoastState::water));

  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.size()==2);
}

TEST_CASE("Synthetize coastlines")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  // square island
  std::vector<Point> coords;
  coords.emplace_back(0, GeoCoord(0,0));
  coords.emplace_back(0, GeoCoord(0,1));
  coords.emplace_back(0, GeoCoord(1,1));
  coords.emplace_back(0, GeoCoord(1,0));
  coastlines.push_back(MkCoastline(0, std::move(coords)));
  coastlines.front()->isArea=true;

  // square bounding polygon intersecting island above
  std::list<WaterIndexProcessor::CoastRef> boundingPolygons;
  coords.clear();
  coords.emplace_back(0, GeoCoord(-0.5,-0.5));
  coords.emplace_back(0, GeoCoord(-0.5,+0.5));
  coords.emplace_back(0, GeoCoord(+0.5,+0.5));
  coords.emplace_back(0, GeoCoord(+0.5,-0.5));
  boundingPolygons.push_back(MkCoastline(1, std::move(coords), WaterIndexProcessor::CoastState::undefined, WaterIndexProcessor::CoastState::unknown));
  coastlines.front()->isArea=true;

  processor.SynthesizeCoastlines(progress, coastlines, boundingPolygons);
  REQUIRE(coastlines.size()==3);

  auto it=coastlines.begin();
  REQUIRE_FALSE((*it)->isArea);
  REQUIRE((*it)->left==WaterIndexProcessor::CoastState::land);
  REQUIRE((*it)->right==WaterIndexProcessor::CoastState::unknown);
  REQUIRE((*it)->coast.size()==3);
  REQUIRE((*it)->coast[0].GetCoord()==GeoCoord(0, 0.5));
  REQUIRE((*it)->coast[1].GetCoord()==GeoCoord(0.5, 0.5));
  REQUIRE((*it)->coast[2].GetCoord()==GeoCoord(0.5, 0));

  ++it;
  REQUIRE_FALSE((*it)->isArea);
  REQUIRE((*it)->left==WaterIndexProcessor::CoastState::water);
  REQUIRE((*it)->right==WaterIndexProcessor::CoastState::unknown);
  REQUIRE((*it)->coast.size()==5);
  REQUIRE((*it)->coast[0].GetCoord()==GeoCoord(0.5, 0));
  REQUIRE((*it)->coast[1].GetCoord()==GeoCoord(0.5, -0.5));
  REQUIRE((*it)->coast[2].GetCoord()==GeoCoord(-0.5, -0.5));
  REQUIRE((*it)->coast[3].GetCoord()==GeoCoord(-0.5, 0.5));
  REQUIRE((*it)->coast[4].GetCoord()==GeoCoord(0, 0.5));

  ++it;
  REQUIRE_FALSE((*it)->isArea);
  REQUIRE((*it)->left==WaterIndexProcessor::CoastState::land);
  REQUIRE((*it)->right==WaterIndexProcessor::CoastState::water);
  REQUIRE((*it)->coast.size()==3);
  REQUIRE((*it)->coast[0].GetCoord()==GeoCoord(0.5, 0));
  REQUIRE((*it)->coast[1].GetCoord()==GeoCoord(0, 0));
  REQUIRE((*it)->coast[2].GetCoord()==GeoCoord(0, 0.5));
}

TEST_CASE("Merge chunked open coastline")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  // build a long open polyline that would be split by BasemapImport
  std::vector<Point> coords;
  size_t             pointCount=1500;

  for (size_t i=0; i<pointCount; i++) {
    coords.emplace_back(0, GeoCoord(0.0, i*0.001));
  }

  // split into 1000 point chunks with one point overlap, like BasemapImport now does
  size_t chunkSize=1000;
  size_t start=0;
  OSMId  id=0;

  while (start<coords.size()) {
    size_t end=std::min(start+chunkSize,coords.size());

    std::vector<Point> chunk(coords.begin()+start,
                             coords.begin()+end);

    coastlines.push_back(MkCoastline(id++,
                                     std::move(chunk),
                                     WaterIndexProcessor::CoastState::water,
                                     WaterIndexProcessor::CoastState::land));

    if (end==coords.size()) {
      break;
    }

    start=end-1;
  }

  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.size()==1);
  REQUIRE_FALSE(coastlines.front()->isArea);
  REQUIRE(coastlines.front()->coast.size()==pointCount);
}

TEST_CASE("Merge chunked closed coastline to area")
{
  WaterIndexProcessor processor;
  SilentProgress progress;
  std::list<WaterIndexProcessor::CoastRef> coastlines;

  // build a long closed ring that would be split by BasemapImport
  std::vector<Point> coords;
  size_t             pointCount=1500;
  static const double pi=std::atan(1.0)*4.0;

  for (size_t i=0; i<pointCount-1; i++) {
    double angle=2.0*pi*i/(pointCount-1);
    coords.emplace_back(0, GeoCoord(std::sin(angle), std::cos(angle)));
  }

  coords.emplace_back(coords.front());

  // split into 1000 point chunks with one point overlap
  size_t chunkSize=1000;
  size_t start=0;
  OSMId  id=0;

  while (start<coords.size()) {
    size_t end=std::min(start+chunkSize,coords.size());

    std::vector<Point> chunk(coords.begin()+start,
                             coords.begin()+end);

    coastlines.push_back(MkCoastline(id++,
                                     std::move(chunk),
                                     WaterIndexProcessor::CoastState::water,
                                     WaterIndexProcessor::CoastState::land));

    if (end==coords.size()) {
      break;
    }

    start=end-1;
  }

  processor.MergeCoastlines(progress, coastlines);
  REQUIRE(coastlines.size()==1);
  REQUIRE(coastlines.front()->isArea);
  // duplicate closing point must be removed by the merge
  REQUIRE(coastlines.front()->coast.size()==pointCount-1);
}

WaterIndexProcessor::CoastlineDataRef MkCoastlineData(Id id,
                                                      std::vector<GeoCoord> points)
{
  auto coastline=std::make_shared<WaterIndexProcessor::CoastlineData>();

  coastline->id=id;
  coastline->isArea=true;
  coastline->isCompletelyInCell=false;
  coastline->points=std::move(points);
  coastline->left=WaterIndexProcessor::CoastState::land;
  coastline->right=WaterIndexProcessor::CoastState::water;

  GetBoundingBox(coastline->points, coastline->boundingBox);

  return coastline;
}

TEST_CASE("FilterEncapsulatedCoastlines removes a real island encapsulated in a bigger island")
{
  WaterIndexProcessor processor;
  SilentProgress progress;

  // "continent" - a counter-clockwise square ring, as a landmass/island is
  // digitised under the natural=coastline convention (sea on the way's
  // right, land on its left; walking with land as the interior is CCW).
  // Verified by hand: shoelace sum over (lon,lat) points (0,0)->(10,0)->
  // (10,10)->(0,10) is +200, i.e. counter-clockwise.
  auto continent=MkCoastlineData(1, {
    GeoCoord(0,0), GeoCoord(0,10), GeoCoord(10,10), GeoCoord(10,0), GeoCoord(0,0)
  });

  // A small ring with the *same* CCW winding, fully nested inside the
  // continent - a redundant island that should still be filtered out.
  auto island=MkCoastlineData(2, {
    GeoCoord(1,1), GeoCoord(1,2), GeoCoord(2,2), GeoCoord(2,1), GeoCoord(1,1)
  });

  std::vector<WaterIndexProcessor::CoastlineDataRef> coastlines{continent, island};

  processor.FilterEncapsulatedCoastlines(progress, coastlines);

  REQUIRE(coastlines.size()==1);
  REQUIRE(coastlines[0]->id==1);
}

TEST_CASE("FilterEncapsulatedCoastlines keeps an enclosed sea encapsulated in a landmass")
{
  WaterIndexProcessor processor;
  SilentProgress progress;

  // Same "continent" ring as above (CCW, land interior).
  auto continent=MkCoastlineData(1, {
    GeoCoord(0,0), GeoCoord(0,10), GeoCoord(10,10), GeoCoord(10,0), GeoCoord(0,0)
  });

  // A smaller ring with the *opposite* (clockwise) winding, fully nested
  // inside the continent - an enclosed sea like the Caspian Sea, digitised
  // the same way a mapper would trace any shoreline (land on the left,
  // water on the right) but around a body of water instead of a landmass.
  // Verified by hand: shoelace sum over (lon,lat) points (3,3)->(3,7)->
  // (7,7)->(7,3) is -32, i.e. clockwise.
  //
  // This must NOT be treated as a redundant island and removed - this is
  // the Caspian Sea bug reproduction. It currently FAILS because
  // FilterEncapsulatedCoastlines only checks left==CoastState::land, which
  // is true for both rings (it's a hardcoded constant, not derived from
  // geometry) and does not yet consult the ring's actual winding.
  auto sea=MkCoastlineData(2, {
    GeoCoord(3,3), GeoCoord(7,3), GeoCoord(7,7), GeoCoord(3,7), GeoCoord(3,3)
  });

  std::vector<WaterIndexProcessor::CoastlineDataRef> coastlines{continent, sea};

  processor.FilterEncapsulatedCoastlines(progress, coastlines);

  REQUIRE(coastlines.size()==2);
}


