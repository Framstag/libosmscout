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
