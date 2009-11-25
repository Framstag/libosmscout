/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/GenWayIndex.h>

#include <map>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Util.h>
#include <osmscout/Way.h>

bool GenerateWayIndex(const ImportParameter& parameter,
                      Progress& progress)
{
  //
  // Analysing distribution of ways in the given interval size
  //

  progress.SetAction("Analysing distribution");

  size_t                     intervalSize=parameter.GetWayIndexIntervalSize();
  FileScanner                scanner;
  std::map<size_t,NodeCount> intervalDist;
  std::map<size_t,size_t>    offsetMap;

  if (!scanner.Open("ways.dat")) {
    progress.Error("Cannot open 'ways.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    long   pos;
    Way    way;

    scanner.GetPos(pos);

    way.Read(scanner);

    if (!scanner.HasError()) {
      intervalDist[way.id/intervalSize]++;

      if (intervalDist[way.id/intervalSize]==1) {
        offsetMap[way.id/intervalSize]=pos;
      }
    }
  }

  scanner.Close();

  /*
  std::cout << "Distribution:" << std::endl;

  for (std::map<size_t,size_t>::const_iterator interval=intervalDist.begin();
       interval!=intervalDist.end();
       ++interval) {
    std::cout << interval->first << ": " << interval->second << std::endl;
  }*/

  progress.Info(NumberToString(intervalDist.size())+" intervals filled");

  //
  // Writing index file
  //

  progress.SetAction("Generating 'way.idx'");

  FileWriter writer;
  size_t     intervalCount=offsetMap.size();

  if (!writer.Open("way.idx")) {
    progress.Error("Cannot create 'ways.idx'");
    return false;
  }

  writer.WriteNumber(intervalSize);  // The size of the interval
  writer.WriteNumber(intervalCount); // The number of intervals we have

  for (std::map<size_t,size_t>::const_iterator offset=offsetMap.begin();
       offset!=offsetMap.end();
       ++offset) {
    writer.WriteNumber(offset->first);               // The interval
    writer.WriteNumber(offset->second);              // The offset
    writer.WriteNumber(intervalDist[offset->first]); // The number of ways
  }

  return !writer.HasError() && writer.Close();
}

