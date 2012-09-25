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

#include <osmscout/util/StopClock.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  std::string RenumberRawDatsGenerator::GetDescription() const
  {
    return "Renumber raw files";
  }

  bool RenumberRawDatsGenerator::RenumberRawNodes(const ImportParameter& parameter,
                                                  Progress& progress)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    nodesCount;
    uint32_t    nodesCopyiedCount=0;
    double      zoomLevel=pow(2.0,(double)parameter.GetRenumberMag());
    size_t      cellCount=zoomLevel*zoomLevel;
    size_t      minIndex=0;
    size_t      maxIndex=cellCount-1;
    size_t      nextId=1;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawnodes.dat"),
                      FileScanner::SequentialScan,
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
      progress.SetAction("Reading nodes in cell range "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex)+" from 'rawnodes.dat'");

      if (!scanner.GotoBegin()) {
        progress.Error(std::string("Error while setting current position in file '")+
                       scanner.GetFilename()+"'");
      }

      if (!scanner.Read(nodesCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      uint32_t                                current=1;
      size_t                                  currentEntries=0;
      std::map<size_t,std::list<FileOffset> > nodesByCell;
      std::map<Id,Id>                         idMap;

      while (current<=nodesCount) {
        RawNode    node;
        FileOffset offset;

        progress.SetProgress(current,nodesCount);

        if (!scanner.GetPos(offset)) {
          progress.Error(std::string("Error while reading current position in file '")+
                         scanner.GetFilename()+"'");

          return false;
        }

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

        if (cellIndex>=minIndex &&
            cellIndex<=maxIndex) {
          nodesByCell[cellIndex].push_back(offset);
          currentEntries++;
        }

        // Reduce cell interval,deleting all already stored nodes beyond the new
        // cell range end.
        if (currentEntries>parameter.GetRenumberBlockSize()) {
          size_t                                            count=0;
          std::map<size_t,std::list<FileOffset> >::iterator cutOff=nodesByCell.end();

          for (std::map<size_t,std::list<FileOffset> >::iterator iter=nodesByCell.begin();
              iter!=nodesByCell.end();
              ++iter) {
            if (count<=parameter.GetRenumberBlockSize() &&
                count+iter->second.size()>parameter.GetRenumberBlockSize()) {
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

      progress.SetAction("Copy renumbered nodes to 'rawnodes2.dat");

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

          if (!scanner.SetPos(*offset)) {
            progress.Error(std::string("Error while setting current position in file '")+
                           scanner.GetFilename()+"'");

            return false;
          }

          if (!node.Read(scanner))  {
            progress.Error(std::string("Error while reading data entry at offset ")+
                           NumberToString(*offset)+
                           " in file '"+
                           scanner.GetFilename()+"'");

            return false;
          }

          idMap[node.GetId()]=nextId;
          node.SetId(nextId);

          nextId++;

          if (!node.Write(writer)) {
            progress.Error(std::string("Error while writing data entry to file '")+
                           scanner.GetFilename()+"'");
          }

          copyCount++;
          nodesCopyiedCount++;
        }
      }

      if (currentEntries==0) {
        progress.Info("No more entries found");
        break;
      }

      progress.SetAction("Write patched ways to 'rawways2.dat'");
      if (!RenumberWayNodes(parameter,
                            progress,
                            idMap)) {
        return false;
      }

      progress.SetAction("Write patched relations to 'rawrels2.dat'");
      if (!RenumberRelationNodes(parameter,
                                 progress,
                                 idMap)) {
        return false;
      }

      if (maxIndex==cellCount-1) {
        // We are finished
        break;
      }

      minIndex=maxIndex+1;
      maxIndex=cellCount-1;
    }

    assert(nodesCount==nodesCopyiedCount);

    return scanner.Close() && writer.Close();
  }

  bool RenumberRawDatsGenerator::RenumberWayNodes(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const std::map<Id,Id>& renumberedNodes)
  {
    FileScanner origScanner;
    FileScanner copyScanner;
    FileWriter  writer;

    if (!origScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                          "rawways.dat"),
                          FileScanner::SequentialScan,
                          parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    copyScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawways2.dat"),
                     FileScanner::SequentialScan,
                     parameter.GetRawWayDataMemoryMaped());

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawways2.tmp"))) {
      progress.Error("Cannot create 'rawways2.tmp'");
      return false;
    }

    uint32_t waysCount;

    if (!origScanner.Read(waysCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (copyScanner.IsOpen()) {
      if (!copyScanner.Read(waysCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }
    }

    writer.Write(waysCount);

    for (size_t i=1; i<=waysCount; i++) {
      progress.SetProgress(i,waysCount);

      RawWay origWay;
      RawWay copyWay;

      if (!origWay.Read(origScanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(i)+" of "+
                       NumberToString(waysCount)+
                       " from file '"+
                       origScanner.GetFilename()+"'");

        return false;
      }

      if (copyScanner.IsOpen()) {
        if (!copyWay.Read(copyScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(i)+" of "+
                         NumberToString(waysCount)+
                         " from file '"+
                         copyScanner.GetFilename()+"'");

          return false;
        }
      }
      else {
        copyWay=origWay;
      }

      std::vector<Id> origNodes=origWay.GetNodes();
      std::vector<Id> copyNodes=copyWay.GetNodes();

      for (size_t n=0; n<origNodes.size(); n++) {
        std::map<Id,Id>::const_iterator renumbered=renumberedNodes.find(origNodes[n]);

        if (renumbered!=renumberedNodes.end()) {
          copyNodes[n]=renumbered->second;
        }
      }

      copyWay.SetNodes(copyNodes);

      if (!copyWay.Write(writer)) {
        progress.Error(std::string("Error while writing data entry ")+
                       NumberToString(i)+" of "+
                       NumberToString(waysCount)+
                       " to file '"+
                       writer.GetFilename()+"'");

        return false;
      }
    }

    if (!origScanner.Close()) {
      progress.Error("Cannot close file '"+origScanner.GetFilename()+"'");

      return false;
    }

    if (copyScanner.IsOpen() &&
        !copyScanner.Close()) {
      progress.Error("Cannot close file '"+copyScanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error("Cannot close file '"+writer.GetFilename()+"'");
      return false;
    }

    RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                               "rawways2.dat"));

    if (!RenameFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawways2.tmp"),
                    AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawways2.dat"))) {
      progress.Error("Cannot rename rawways2.tmp to rawways2.dat");
      return false;
    }


    return true;
  }

  bool RenumberRawDatsGenerator::RenumberRelationNodes(const ImportParameter& parameter,
                                                       Progress& progress,
                                                       const std::map<Id,Id>& renumberedNodes)
  {
    FileScanner origScanner;
    FileScanner copyScanner;
    FileWriter  writer;

    if (!origScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                          "rawrels.dat"),
                          FileScanner::SequentialScan,
                          true/*TODO*/)) {
      progress.Error("Cannot open 'rawrels.dat'");
      return false;
    }

    copyScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawrels2.dat"),
                     FileScanner::SequentialScan,
                     true/*TODO*/);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawrels2.tmp"))) {
      progress.Error("Cannot create 'rawrels2.tmp'");
      return false;
    }

    uint32_t relsCount;

    if (!origScanner.Read(relsCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (copyScanner.IsOpen()) {
      if (!copyScanner.Read(relsCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }
    }

    writer.Write(relsCount);

    for (size_t i=1; i<=relsCount; i++) {
      progress.SetProgress(i,relsCount);

      RawRelation origRelation;
      RawRelation copyRelation;

      if (!origRelation.Read(origScanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(i)+" of "+
                       NumberToString(relsCount)+
                       " from file '"+
                       origScanner.GetFilename()+"'");

        return false;
      }

      if (copyScanner.IsOpen()) {
        if (!copyRelation.Read(copyScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(i)+" of "+
                         NumberToString(relsCount)+
                         " from file '"+
                         copyScanner.GetFilename()+"'");

          return false;
        }
      }
      else {
        copyRelation=origRelation;
      }

      for (size_t m=0; m<origRelation.members.size(); m++) {
        if (origRelation.members[m].type==RawRelation::memberNode) {
          std::map<Id,Id>::const_iterator renumbered=renumberedNodes.find(origRelation.members[m].id);

          if (renumbered!=renumberedNodes.end()) {
            copyRelation.members[m].id=renumbered->second;
          }
        }
      }

      if (!copyRelation.Write(writer)) {
        progress.Error(std::string("Error while writing data entry ")+
                       NumberToString(i)+" of "+
                       NumberToString(relsCount)+
                       " to file '"+
                       writer.GetFilename()+"'");

        return false;
      }
    }

    if (!origScanner.Close()) {
      progress.Error("Cannot close file '"+origScanner.GetFilename()+"'");

      return false;
    }

    if (copyScanner.IsOpen() &&
        !copyScanner.Close()) {
      progress.Error("Cannot close file '"+copyScanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error("Cannot close file '"+writer.GetFilename()+"'");
      return false;
    }


    RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                               "rawrels2.dat"));

    if (!RenameFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawrels2.tmp"),
                    AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawrels2.dat"))) {
      progress.Error("Cannot rename rawrels2.tmp to rawrels2.dat");
      return false;
    }


    return true;
  }

  bool RenumberRawDatsGenerator::RenumberRawWays(const ImportParameter& parameter,
                                                 Progress& progress)
  {
    FileScanner nodeScanner;
    uint32_t    nodesCount;

    if (!nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                          "rawnodes2.dat"),
                          FileScanner::SequentialScan,
                          parameter.GetRawNodeDataMemoryMaped())) {
      progress.Error("Cannot open 'rawnodes2.dat'");
      return false;
    }

    if (!nodeScanner.Read(nodesCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!nodeScanner.Close()) {
      progress.Error("Cannot close file '"+nodeScanner.GetFilename()+"'");
    }

    FileScanner wayScanner;
    FileWriter  writer;
    uint32_t    waysCount;
    uint32_t    waysCopyiedCount=0;
    size_t      cellCount=nodesCount/1000+1;
    size_t      minIndex=0;
    size_t      maxIndex=nodesCount/1000;
    size_t      nextId=1;

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "rawways2.dat"),
                         FileScanner::SequentialScan,
                         parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways2.dat'");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawways3.dat"))) {
      progress.Error("Cannot create 'rawways3.dat'");
      return false;
    }

    if (!wayScanner.Read(waysCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    writer.Write(waysCount);

    while (true) {
      progress.SetAction("Reading ways in cell range from 'rawways2.dat'");

      if (!wayScanner.GotoBegin()) {
        progress.Error(std::string("Error while setting current position in file '")+
                       wayScanner.GetFilename()+"'");
      }

      if (!wayScanner.Read(waysCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      progress.Info("Scanning cells in range "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));

      uint32_t                                current=1;
      size_t                                  currentEntries=0;
      std::map<size_t,std::list<FileOffset> > waysByCell;
      std::map<Id,Id>                         idMap;

      while (current<=waysCount) {
        RawWay     way;
        FileOffset offset;

        progress.SetProgress(current,waysCount);

        if (!wayScanner.GetPos(offset)) {
          progress.Error(std::string("Error while reading current position in file '")+
                         wayScanner.GetFilename()+"'");

          return false;
        }

        if (!way.Read(wayScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(nodesCount)+
                         " in file '"+
                         wayScanner.GetFilename()+"'");
          return false;
        }

        Id minNodeId=way.GetNodeId(0);

        for (size_t i=1; i<way.GetNodeCount(); i++) {
          minNodeId=std::min(minNodeId,way.GetNodeId(i));
        }

        size_t cellIndex=minNodeId/1000;

        if (cellIndex>=minIndex &&
            cellIndex<=maxIndex) {
          waysByCell[cellIndex].push_back(offset);
          currentEntries++;
        }

        // Reduce cell interval,deleting all already stored nodes beyond the new
        // cell range end.
        if (currentEntries>parameter.GetRenumberBlockSize()) {
          size_t                                            count=0;
          std::map<size_t,std::list<FileOffset> >::iterator cutOff=waysByCell.end();

          for (std::map<size_t,std::list<FileOffset> >::iterator iter=waysByCell.begin();
              iter!=waysByCell.end();
              ++iter) {
            if (count<=parameter.GetRenumberBlockSize() &&
                count+iter->second.size()>parameter.GetRenumberBlockSize()) {
              cutOff=iter;
              break;
            }
            else {
              count+=iter->second.size();
              maxIndex=iter->first;
            }
          }

          assert(cutOff!=waysByCell.end());

          currentEntries=count;
          waysByCell.erase(cutOff,waysByCell.end());

          progress.Debug("Reducing cell range to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
        }

        current++;
      }

      if (maxIndex<cellCount-1) {
        progress.Info("Cell range was reduced to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
      }

      progress.SetAction("Copy renumbered ways to 'rawways3.dat");

      size_t copyCount=0;
      for (std::map<size_t,std::list<FileOffset> >::iterator iter=waysByCell.begin();
          iter!=waysByCell.end();
          ++iter) {

        iter->second.sort();

        for (std::list<FileOffset>::const_iterator offset=iter->second.begin();
            offset!=iter->second.end();
            ++offset) {
          progress.SetProgress(copyCount,currentEntries);

          RawWay way;

          if (!wayScanner.SetPos(*offset)) {
            progress.Error(std::string("Error while setting current position in file '")+
                           wayScanner.GetFilename()+"'");

            return false;
          }

          if (!way.Read(wayScanner))  {
            progress.Error(std::string("Error while reading data entry at offset ")+
                           NumberToString(*offset)+
                           " in file '"+
                           wayScanner.GetFilename()+"'");

            return false;
          }

          idMap[way.GetId()]=nextId;
          way.SetId(nextId);

          nextId++;

          if (!way.Write(writer)) {
            progress.Error(std::string("Error while writing data entry to file '")+
                           wayScanner.GetFilename()+"'");
          }

          copyCount++;
          waysCopyiedCount++;
        }
      }

      if (currentEntries==0) {
        progress.Info("No more entries found");
        break;
      }

      progress.SetAction("Write patched relations to 'rawrels3.dat'");
      if (!RenumberRelationWays(parameter,
                                progress,
                                idMap)) {
        return false;
      }

      if (maxIndex==cellCount-1) {
        // We are finished
        break;
      }

      minIndex=maxIndex+1;
      maxIndex=cellCount-1;
    }

    assert(waysCount==waysCopyiedCount);

    return wayScanner.Close() && writer.Close();

    return true;
  }

  bool RenumberRawDatsGenerator::RenumberRelationWays(const ImportParameter& parameter,
                                                      Progress& progress,
                                                      const std::map<Id,Id>& renumberedNodes)
  {
    FileScanner origScanner;
    FileScanner copyScanner;
    FileWriter  writer;

    if (!origScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                          "rawrels2.dat"),
                          FileScanner::SequentialScan,
                          true/*TODO*/)) {
      progress.Error("Cannot open 'rawrels2.dat'");
      return false;
    }

    copyScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawrels3.dat"),
                     FileScanner::SequentialScan,
                     true/*TODO*/);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawrels3.tmp"))) {
      progress.Error("Cannot create 'rawrels3.tmp'");
      return false;
    }

    uint32_t waysCount;

    if (!origScanner.Read(waysCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (copyScanner.IsOpen()) {
      if (!copyScanner.Read(waysCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }
    }

    writer.Write(waysCount);

    for (size_t i=1; i<=waysCount; i++) {
      RawRelation origRelation;
      RawRelation copyRelation;

      if (!origRelation.Read(origScanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(i)+" of "+
                       NumberToString(waysCount)+
                       " from file '"+
                       origScanner.GetFilename()+"'");

        return false;
      }

      if (copyScanner.IsOpen()) {
        if (!copyRelation.Read(copyScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(i)+" of "+
                         NumberToString(waysCount)+
                         " from file '"+
                         copyScanner.GetFilename()+"'");

          return false;
        }
      }
      else {
        copyRelation=origRelation;
      }

      for (size_t m=0; m<origRelation.members.size(); m++) {
        if (origRelation.members[m].type==RawRelation::memberWay) {
          std::map<Id,Id>::const_iterator renumbered=renumberedNodes.find(origRelation.members[m].id);

          if (renumbered!=renumberedNodes.end()) {
            copyRelation.members[m].id=renumbered->second;
          }
        }
      }

      if (!copyRelation.Write(writer)) {
        progress.Error(std::string("Error while writing data entry ")+
                       NumberToString(i)+" of "+
                       NumberToString(waysCount)+
                       " to file '"+
                       writer.GetFilename()+"'");

        return false;
      }
    }

    if (!origScanner.Close()) {
      progress.Error("Cannot close file '"+origScanner.GetFilename()+"'");

      return false;
    }

    if (copyScanner.IsOpen() &&
        !copyScanner.Close()) {
      progress.Error("Cannot close file '"+copyScanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error("Cannot close file '"+writer.GetFilename()+"'");
      return false;
    }


    RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                               "rawwrels3.dat"));

    if (!RenameFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawrels3.tmp"),
                    AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawrels3.dat"))) {
      progress.Error("Cannot rename rawrels3.tmp to rawrels3.dat");
      return false;
    }


    return true;
  }

  bool RenumberRawDatsGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    if (parameter.GetRenumberIds()) {
      /*
      if (!RenumberRawNodes(parameter,
                            progress)) {
        return false;
      }*/

      if (!RenumberRawWays(parameter,
                           progress)) {
        return false;
      }

      progress.SetAction("Copying renumbered files to original raw files");

      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawnodes.dat"));
      RenameFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawnodes2.dat"),
                 AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawnodes.dat"));

      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawways.dat"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawways2.dat"));
      RenameFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawways3.dat"),
                 AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawways.dat"));

      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawrels.dat"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawrels2.dat"));
      RenameFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawrels3.dat"),
                 AppendFileToDir(parameter.GetDestinationDirectory(),
                                 "rawrels.dat"));
    }
    else {
      progress.SetAction("Deleting leftover renumber files");

      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawnodes2.tmp"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawnodes2.dat"));

      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawways2.tmp"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawways2.dat"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawways3.tmp"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawways3.dat"));

      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawrels2.tmp"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawrels2.dat"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawrels3.tmp"));
      RemoveFile(AppendFileToDir(parameter.GetDestinationDirectory(),
                 "rawrels3.dat"));
    }

    return true;
  }
}
