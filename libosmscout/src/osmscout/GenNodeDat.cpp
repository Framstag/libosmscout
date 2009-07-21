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
#include <fstream>
#include <iostream>
#include <map>

#include <osmscout/Tiles.h>
#include <osmscout/RawNode.h>
#include <osmscout/Node.h>

bool GenerateNodeDat()
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  std::cout << "Generate nodes.dat..." << std::endl;

  std::ifstream in;
  std::ofstream out;

  in.open("rawnodes.dat",std::ios::in|std::ios::binary);

  if (!in) {
    return false;
  }

  out.open("nodes.dat",std::ios::out|std::ios::trunc|std::ios::binary);

  if (!out) {
    return false;
  }

  while (in) {
    RawNode rawNode;
    Node    node;

    rawNode.Read(in);

    if (rawNode.type!=typeIgnore) {
      node.id=rawNode.id;
      node.type=rawNode.type;
      node.lat=rawNode.lat;
      node.lon=rawNode.lon;
      node.tags=rawNode.tags;

      node.Write(out);
    }
  }

  in.close();
  out.close();

  return true;
}

