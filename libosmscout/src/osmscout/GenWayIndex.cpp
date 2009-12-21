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
#include <iostream>
bool GenerateWayIndex(const ImportParameter& parameter,
                      Progress& progress)
{
  //
  // Analysing distribution of ways in the given interval size
  //

  progress.SetAction("Analysing distribution");

  size_t                     intervalSize=parameter.GetWayIndexIntervalSize();
  FileScanner                scanner;
  size_t                     ways=0;
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

      ways++;
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

  /*

  if (writer.HasError() || !writer.Close()) {
    return false;
  }

  size_t levels=1;
  size_t tmp;

  tmp=ways;
  while (tmp/parameter.GetIndexPageSize()>0) {
    tmp=tmp/parameter.GetIndexPageSize();
    levels++;
  }

  //std::cout << "We have " << levels << " level(s)" << " for indexing " << nodes << " entries" << std::endl;


  // NEW IMPLEMENTATION!!!

  //
  // Writing index2 file
  //

  progress.SetAction("Generating 'way2.idx'");

  if (!writer.Open("way2.idx")) {
    progress.Error("Cannot create 'way2.idx'");
    return false;
  }

  if (!scanner.Open("ways.dat")) {
    progress.Error("Cannot open 'ways.dat'");
    return false;
  }

  std::vector<Id>            startingIds;
  std::vector<unsigned long> pageStarts;
  long                       lastLevelPageStart;

  writer.WriteNumber(levels); // Number of levels
  writer.WriteNumber(parameter.GetIndexPageSize()); // Size of index page
  writer.Write(ways);        // Number of nodes

  writer.GetPos(lastLevelPageStart);

  writer.Write((unsigned long)0); // Write the starting position of the last page

  size_t currentEntry=0;
  size_t lastId=0;
  long   lastPos=0;

  progress.Info(std::string("Level ")+NumberToString(levels)+" entries "+NumberToString(ways));

  while (!scanner.HasError()) {
    long pos;

    scanner.GetPos(pos);

    Way way;

    way.Read(scanner);

    if (!scanner.HasError()) {
      if (currentEntry%parameter.GetIndexPageSize()==0) {
        long pageStart;

        writer.GetPos(pageStart);

        //std::cout << levels << "-" << pageStart << " " << way.id << " " << pos << std::endl;
        writer.WriteNumber((unsigned long)pos);
        writer.WriteNumber(way.id);
        //writer.Write((unsigned long)pos);
        //writer.Write(way.id);
        startingIds.push_back(way.id);
        pageStarts.push_back(pageStart);
      }
      else {
        long pageStart;

        writer.GetPos(pageStart);

        //std::cout << levels << " " << pageStart << " " << way.id << " " << lastId << " " << pos << " " << lastPos << std::endl;
        writer.WriteNumber((unsigned long)(pos-lastPos));
        writer.WriteNumber(way.id-lastId);
        //writer.Write((unsigned long)(pos-lastPos));
        //writer.Write(way.id-lastId);
      }

      lastPos=pos;
      lastId=way.id;

      currentEntry++;
    }
  }

  levels--;

  while (levels>0) {
    std::vector<Id>            si(startingIds);
    std::vector<unsigned long> po(pageStarts);

    startingIds.clear();
    pageStarts.clear();

    progress.Info(std::string("Level ")+NumberToString(levels)+" entries "+NumberToString(si.size()));

    for (size_t i=0; i<si.size(); i++) {
      if (i%parameter.GetIndexPageSize()==0) {
        long pos;

        writer.GetPos(pos);

        startingIds.push_back(si[i]);
        pageStarts.push_back(pos);
      }

      if (i==0) {
        //std::cout << levels << " " << si[i] << " " << po[i] << std::endl;
        writer.WriteNumber(si[i]);
        writer.WriteNumber(po[i]);
        //writer.Write(si[i]);
        //writer.Write(po[i]);
      }
      else {
        //std::cout << levels << " " << si[i] << " " << po[i] << " " << po[i]-po[i-1] << std::endl;
        writer.WriteNumber(si[i]-si[i-1]);
        writer.WriteNumber(po[i]-po[i-1]);
        //writer.Write(si[i]-si[i-1]);
        //writer.Write(po[i]-po[i-1]);
      }
    }

    levels--;
  }

  writer.SetPos(lastLevelPageStart);

  std::cout << "Starting pos of last level index is " << pageStarts[0] << std::endl;
  writer.Write((unsigned long)pageStarts[0]);

  scanner.Close();*/

  return !writer.HasError() && writer.Close();
}

