/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/import/RenumberRawDats.h>

#include <osmscout/DataFile.h>

#include <osmscout/Util.h>

#include <osmscout/util/StopClock.h>

#include <osmscout/import/RawNode.h>

namespace osmscout {

  static const size_t zoomLevel=4096;
  static const size_t maxEntries=60000000;

  std::string RenumberRawDatsGenerator::GetDescription() const
  {
    return "Renumber raw files";
  }

  bool RenumberRawDatsGenerator::RenumberRawNodes(const ImportParameter& parameter,
                                                  Progress& progress)
  {
    FileScanner                 scanner;
    FileWriter                  writer;
    uint32_t                    nodesCount;
    size_t                      cellCount=zoomLevel*zoomLevel;
    std::map<size_t,std::list<FileOffset> > nodesByCell;
    std::map<Id,Id>             idMap;
    size_t                      minIndex=0;
    size_t                      maxIndex=cellCount-1;
    size_t                      nextId=1;

    progress.SetAction("Scan 'rawnodes.dat");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawnodes.dat"),
                                      true,
                                      parameter.GetRawNodeDataMemoryMaped())) {
      progress.Error("Cannot open 'rawnodes.dat'");
      return false;
    }

    if (!scanner.Read(nodesCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawnodes2.dat"))) {
      progress.Error("Cannot create 'rawnodes2.dat'");
      return false;
    }

    writer.Write(nodesCount);

    while (true) {
      scanner.GotoBegin();

      if (!scanner.Read(nodesCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      progress.Info("Scanning cells "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));

      uint32_t current=1;
      size_t   currentEntries=0;

      while (current<nodesCount) {
        RawNode    node;
        FileOffset offset;

        progress.SetProgress(current,nodesCount);

        scanner.GetPos(offset);

        if (!node.Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(nodesCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        size_t cellY=(size_t)(node.GetLat()+90.0/zoomLevel);
        size_t cellX=(size_t)(node.GetLon()+180.0/zoomLevel);
        size_t cellIndex=cellY*zoomLevel+cellX;

        if (cellIndex>=minIndex && cellIndex<=maxIndex) {
          nodesByCell[cellIndex].push_back(offset);
          currentEntries++;
        }

        if (currentEntries>maxEntries) {
          size_t                                            count=0;
          std::map<size_t,std::list<FileOffset> >::iterator cutOff=nodesByCell.end();

          for (std::map<size_t,std::list<FileOffset> >::iterator iter=nodesByCell.begin();
              iter!=nodesByCell.end();
              ++iter) {
            if (count<=maxEntries && count+iter->second.size()>maxEntries) {
              cutOff=iter;
              break;
            }
            else {
              count+=iter->second.size();
              maxIndex=iter->first;
            }
          }

          assert(cutOff!=nodesByCell.end());

          currentEntries=count;
          nodesByCell.erase(cutOff,nodesByCell.end());

          progress.Debug("Reducing cell range to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
        }

        current++;
      }

      if (maxIndex<cellCount-1) {
        progress.Info("Cell range was reduced to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
      }

      progress.Info("Copying entries");

      size_t copyCount=0;
      for (std::map<size_t,std::list<FileOffset> >::iterator iter=nodesByCell.begin();
          iter!=nodesByCell.end();
          ++iter) {

        iter->second.sort();

        for (std::list<FileOffset>::const_iterator offset=iter->second.begin();
            offset!=iter->second.end();
            ++offset) {
          progress.SetProgress(copyCount,currentEntries);

          RawNode node;

          scanner.SetPos(*offset);

          node.Read(scanner);

          idMap[node.GetId()]=nextId;
          node.SetId(nextId);

          nextId++;

          node.Write(writer);

          copyCount++;
        }
      }

      if (currentEntries==0) {
        progress.Info("No more entries found");
        break;
      }

      if (maxIndex==cellCount-1) {
        progress.Info("All cells scanned");
        break;
      }

      minIndex=maxIndex+1;
      maxIndex=cellCount-1;

      nodesByCell.clear();
      currentEntries=0;

      idMap.clear();
    }

    return scanner.Close() && writer.Close();
  }

  bool RenumberRawDatsGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    //return true;

    if (!RenumberRawNodes(parameter,
                          progress)) {
      return false;
    }

    return true;
  }
}

