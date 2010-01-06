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

#include <osmscout/GenAreaWayIndex2.h>

#include <cassert>
#include <cmath>
#include <vector>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Util.h>
#include <osmscout/Way.h>

#include <iostream>

struct Leaf
{
  long            offset;
  std::list<long> dataOffsets;
  long            children[4];

  Leaf()
  {
    offset=0;
    children[0]=0;
    children[1]=0;
    children[2]=0;
    children[3]=0;
  }
};

bool GenerateAreaWayIndex2(const ImportParameter& parameter,
                           Progress& progress)
{
  //
  // Count the number of objects per index level
  //

  progress.SetAction("Analysing distribution");

  FileScanner           scanner;
  size_t                ways=0;     // Number of ways found
  size_t                consumed=0; // Number of ways consumed
  std::vector<double>   cellWidth;
  std::vector<double>   cellHeight;
  std::map<size_t,Leaf> leafs;
  std::map<size_t,Leaf> newLeafs;

  cellWidth.resize(parameter.GetAreaAreaIndexMaxMag()+1);
  cellHeight.resize(parameter.GetAreaAreaIndexMaxMag()+1);

  for (size_t i=0; i<cellWidth.size(); i++) {
    cellWidth[i]=360/pow(2,i);
  }

  for (size_t i=0; i<cellHeight.size(); i++) {
    cellHeight[i]=180/pow(2,i);
  }

  for (size_t i=0; i<cellWidth.size(); i++) {
    std::cout << "Cell dimension for mag " << i << ": " << cellWidth[i] << "x" << cellHeight[i] << " (1/" << pow(2,i) << ")" << std::endl;
  }

  //
  // Writing index file
  //

  progress.SetAction("Generating 'areaway2.idx'");

  FileWriter writer;

  if (!writer.Open("areaway2.idx")) {
    progress.Error("Cannot create 'areaway2.idx'");
    return false;
  }

  writer.WriteNumber(parameter.GetAreaAreaIndexMaxMag()); // MaxMag

  int l=parameter.GetAreaAreaIndexMaxMag();

  while (l>=0) {
    size_t levelEntries=0;

    std::cout << "Storing level " << l << "..." << std::endl;

    newLeafs.clear();

    for (std::map<size_t,Leaf>::iterator leaf=leafs.begin();
         leaf!=leafs.end();
         ++leaf) {
      size_t xc=leaf->first%Pow(2,l+1);
      size_t yc=leaf->first/Pow(2,l+1);

      size_t index;

      if (yc%2!=0) {
        if (xc%2==0) {
          index=0;
        }
        else {
          index=1;
        }
      }
      else {
        if (xc%2==0) {
          index=2;
        }
        else {
          index=3;
        }
      }

      newLeafs[yc/2*Pow(2,l)+xc/2].children[index]=leaf->second.offset;
    }

    leafs=newLeafs;

    if (ways==0 ||
        (ways>0 && ways>consumed)) {
      std::cout << " Scanning ways.dat for ways of idex level " << l << "..." << std::endl;
      if (!scanner.Open("ways.dat")) {
        progress.Error("Cannot open 'ways.dat'");
        return false;
      }

      ways=0;
      while (!scanner.HasError()) {
        long offset;
        Way   way;

        scanner.GetPos(offset);
        way.Read(scanner);

        if (!scanner.HasError()) {
          ways++;

          if (way.nodes.size()==0) {
            continue;
          }

          // Bounding box
          double minLon;
          double maxLon;
          double minLat;
          double maxLat;

          minLon=way.nodes[0].lon;
          maxLon=way.nodes[0].lon;
          minLat=way.nodes[0].lat;
          maxLat=way.nodes[0].lat;

          for (size_t i=1; i<way.nodes.size(); i++) {
            minLon=std::min(minLon,way.nodes[i].lon);
            maxLon=std::max(maxLon,way.nodes[i].lon);
            minLat=std::min(minLat,way.nodes[i].lat);
            maxLat=std::max(maxLat,way.nodes[i].lat);
          }

          int    level=parameter.GetAreaAreaIndexMaxMag();
          double xc=0;
          double yc=0;
          double clon; // longitude of area center
          double clat; // latitude of area center

          minLon+=180;
          maxLon+=180;
          minLat+=90;
          maxLat+=90;

          clon=minLon+(maxLon-minLon)/2;
          clat=minLat+(maxLat-minLat)/2;

          while (level>=0) {
            xc=floor(clon/cellWidth[level]);
            yc=floor(clat/cellHeight[level]);

            if (minLon>=cellWidth[level]*xc &&
                maxLon<cellWidth[level]*(xc+1) &&
                minLat>=cellHeight[level]*yc &&
                maxLat<cellHeight[level]*(yc+1)) {
              break;
            }

            level--;
          }

          if (level==l) {
            if (l==14 && ways==1) {
              std::cout << way.GetName() << " " << way.GetRefName() << " " << clon << " " << clat << " " << xc << " " << yc << std::endl;
            }
            leafs[yc*Pow(2,level)+xc].dataOffsets.push_back(offset);
            levelEntries++;
            consumed++;
          }
        }
      }

      scanner.Close();
    }

    std::cout << " Writing " << leafs.size() << " entries with " << levelEntries << " ways to index of level " << l << "..." << std::endl;
    // Store leafs
    writer.WriteNumber(leafs.size()); // Number of leafs
    for (std::map<size_t,Leaf>::iterator leaf=leafs.begin();
         leaf!=leafs.end();
         ++leaf) {
      writer.GetPos(leaf->second.offset);
      //writer.WriteNumber(leaf->first);

      if (l<parameter.GetAreaAreaIndexMaxMag()) {
        // TODO: Is writer.Write better?
        writer.WriteNumber(leaf->second.children[0]);
        writer.WriteNumber(leaf->second.children[1]);
        writer.WriteNumber(leaf->second.children[2]);
        writer.WriteNumber(leaf->second.children[3]);
      }
      writer.WriteNumber(leaf->second.dataOffsets.size());
      for (std::list<long>::const_iterator o=leaf->second.dataOffsets.begin();
           o!=leaf->second.dataOffsets.end();
           o++) {
        // TODO: Is writer.Write better?
        writer.WriteNumber((size_t)*o);
      }
    }

    l--;
  }

  return !writer.HasError() && writer.Close();

  return true;
}
