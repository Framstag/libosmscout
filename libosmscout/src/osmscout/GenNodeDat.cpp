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

#include <osmscout/GenNodeDat.h>

#include <cmath>
#include <iostream>
#include <map>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Tiles.h>
#include <osmscout/RawNode.h>
#include <osmscout/Node.h>

bool GenerateNodeDat(const ImportParameter& parameter,
                     Progress& progress)
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  progress.SetAction("Generating nodes.dat");

  FileScanner scanner;
  FileWriter  writer;

  if (!scanner.Open("rawnodes.dat")) {
    progress.Error("Cannot open 'rawnodes.dat'");
    return false;
  }

  if (!writer.Open("nodes.dat")) {
    progress.Error("Cannot create 'nodes.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    RawNode rawNode;
    Node    node;

    rawNode.Read(scanner);

    if (rawNode.type!=typeIgnore) {
      node.id=rawNode.id;
      node.type=rawNode.type;
      node.lat=rawNode.lat;
      node.lon=rawNode.lon;
      node.tags=rawNode.tags;

      node.Write(writer);
    }
  }

  scanner.Close();
  writer.Close();

  return true;
}

