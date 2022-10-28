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

#include <TestMain.h>

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
                                          WaterIndexProcessor::CoastState right=WaterIndexProcessor::CoastState::water,
                                          WaterIndexProcessor::CoastState left=WaterIndexProcessor::CoastState::land)
{
  if (coords.empty()) {
    return std::make_shared<WaterIndexProcessor::Coast>(WaterIndexProcessor::Coast{id, false, 0, 0, coords, right, left});
  }
  bool isArea=coords.front().IsIdentical(coords.back());
  Id frontNodeId = coords.front().GetId();
  Id backNodeId = coords.back().GetId();
  return std::make_shared<WaterIndexProcessor::Coast>(WaterIndexProcessor::Coast{id, isArea, frontNodeId, backNodeId, coords, right, left});
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
